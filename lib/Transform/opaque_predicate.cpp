//=== opaque_predicate.cpp - Manufactures opaque predicates ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Opaque predicates are based on the equations given in the paper at
// http://crypto.cs.mcgill.ca/~garboit/sp-paper.pdf
// TODO: Natural Number and exponential formulaes

#define DEBUG_TYPE "opaque"
#include "Transform/opaque_predicate.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include <chrono>
#include <random>
#include <cassert>
using namespace llvm;

static cl::opt<unsigned> opaqueGlobal(
    "opaque-global", cl::init(4),
    cl::desc("Number of global variables in a module for opaque predicates"));

static cl::opt<std::string> opaqueSeed(
    "opaque-seed", cl::init(""),
    cl::desc("Seed for random number generator. Defaults to system time"));

bool OpaquePredicate::runOnModule(Module &M) {
  // Seed engine and create distribution
  if (!opaqueSeed.empty()) {
    std::seed_seq seed(opaqueSeed.begin(), opaqueSeed.end());
    engine.seed(seed);
  } else {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    engine.seed(seed);
  }

  // Create globals
  std::vector<GlobalVariable *> globals = prepareModule(M);

  std::uniform_int_distribution<int> distribution;
  std::uniform_int_distribution<int> distributionType(0, 1);

  LLVMContext &context = M.getContext();
  unsigned metaKind = context.getMDKindID(unreachableMarkName);

  for (auto &function : M) {
    DEBUG(errs() << "\tFunction " << function.getName() << "\n");
    for (auto &block : function) {
      TerminatorInst *terminator = block.getTerminator();

      PredicateType type = getInstructionType(*terminator, stubName);
      if (type == PredicateNone) {
        continue;
      }

      BranchInst *branch = dyn_cast<BranchInst>(terminator);
      assert(branch && "Terminator should be a branch!");
      assert(branch->isConditional() &&
             "Stub terminator should be conditional!");

      Value *condition = branch->getCondition();
      FCmpInst *compare = dyn_cast<FCmpInst>(condition);
      assert(compare && "Stub condition should be FCmpInst");
      assert(compare->getNumOperands() == 2 &&
             "Penultimate instruction should have two operands");
      assert(compare->getPredicate() == FCmpInst::FCMP_TRUE &&
             "Penultimate instruction should have an always true predicate");

      DEBUG(errs() << "\t\tFound: " << type << "\n");
      assert(type != PredicateIndeterminate &&
             "Indeterminate predicate not supported yet");

      BasicBlock *trueBlock = branch->getSuccessor(0);
      BasicBlock *falseBlock = branch->getSuccessor(1);

      bool mark = true;
      if (branch->getMetadata(metaKind)) {
        mark = false;
      }

      branch->eraseFromParent();
      compare->eraseFromParent();

      PredicateType createdType;
      if (type == PredicateTrue) {
        createTrue(&block, trueBlock, falseBlock, globals, [&]{
          return distribution(engine);
        });
        createdType = PredicateTrue;
      } else if (type == PredicateFalse) {
        createFalse(&block, trueBlock, falseBlock, globals, [&]{
          return distribution(engine);
        });
        createdType = PredicateTrue;
      } else {
        createdType = create(&block, trueBlock, falseBlock, globals, [&]{
          return distribution(engine);
        },
                             [&]()->OpaquePredicate::PredicateType{
          return static_cast<OpaquePredicate::PredicateType>(
              distributionType(engine));
        });
        DEBUG(errs() << "\t\tOpaque Predicate Created: " << createdType
                     << "\n");
      }

      // Check if we want any marking
      if (mark) {
        switch (createdType) {
        case PredicateTrue:
          cleanDebug(*falseBlock);
          tagInstruction(*(falseBlock->begin()), unreachableName,
                         PredicateTrue);
          break;
        case PredicateFalse:
          cleanDebug(*trueBlock);
          tagInstruction(*(trueBlock->begin()), unreachableName,
                         PredicateFalse);
          break;
        default:
          llvm_unreachable("Unsupported predicate type");
        }
      }
      DEBUG_WITH_TYPE("opaque_cfg", function.viewCFG());
    }
  }
  return true;
}

// TODO: Use some runtime randomniser? Maybe?
Value *OpaquePredicate::advanceGlobal(BasicBlock *block, GlobalVariable *global,
                                      OpaquePredicate::Randomner randomner) {
  assert(global && "Null global pointer");
  DEBUG(errs() << "[Opaque Predicate] Randomly advancing global\n");
  DEBUG(errs() << "\tCreating Random Constant\n");
  int randomValue = randomner();

  while (!randomValue) {
    randomValue = randomner();
  }

  Value *random = ConstantInt::get(Type::getInt32Ty(block->getContext()),
                                   randomValue, true);
  DEBUG(errs() << "\tLoading global\n");
  LoadInst *load = new LoadInst((Value *)global, "", block);
  DEBUG(errs() << "\tAdding global\n");

  Instruction::BinaryOps op = Instruction::Add;
  switch (randomner() % 3) {
  case 0:
    op = Instruction::Add;
    break;
  case 1:
    op = Instruction::Sub;
    break;
  case 2:
    op = Instruction::Mul;
    break;
  }

  BinaryOperator *advance =
      BinaryOperator::Create(op, (Value *)load, (Value *)random, "", block);
  DEBUG(errs() << "\tStoring global\n");
  Value *zero =
      ConstantInt::get(Type::getInt32Ty(block->getContext()), 0, true);
  ICmpInst *compare = new ICmpInst(*block, CmpInst::ICMP_EQ, advance, zero);
  // SelectInst to ensure non zero
  SelectInst *returnValue =
      SelectInst::Create(compare, random, advance, "", block);
  new StoreInst(returnValue, (Value *)global, block);
  return (Value *)returnValue;
}

// 7y^2 -1 != x^2 for all x, y in Z
Value *OpaquePredicate::formula0(BasicBlock *block, Value *x, Value *y,
                                 OpaquePredicate::PredicateType type) {

  assert(type != OpaquePredicate::PredicateIndeterminate &&
         "Formula 0 does not support indeterminate!");

  // 96 bit integer type
  Type *intType = (Type *)Type::getIntNTy(block->getContext(), 96);

  // Sign extend to 96 bits
  Value *x1 = (Value *)new SExtInst(x, intType, "", block);
  Value *y1 = (Value *)new SExtInst(y, intType, "", block);

  Value *seven =
      ConstantInt::get(intType, 7, false);
  Value *one =
      ConstantInt::get(intType, 1, false);
  // x^2
  Value *x2 =
      (Value *)BinaryOperator::Create(Instruction::Mul, x1, x1, "", block);
  // y^2
  Value *y2 =
      (Value *)BinaryOperator::Create(Instruction::Mul, y1, y1, "", block);
  // 7y^2
  Value *y3 =
      (Value *)BinaryOperator::Create(Instruction::Mul, y2, seven, "", block);
  // 7y^2 - 1
  Value *y4 =
      (Value *)BinaryOperator::Create(Instruction::Sub, y3, one, "", block);

  Value *condition;
  // Compare
  if (type == OpaquePredicate::PredicateTrue)
    condition = CmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_NE, x2, y4,
                                "", block);
  else
    condition = CmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_EQ, x2, y4,
                                "", block);

  return condition;
}

// (x^3 - x) % 3 == 0 for all x in Z
Value *OpaquePredicate::formula1(BasicBlock *block, Value *x, Value *y1,
                                 OpaquePredicate::PredicateType type) {
  // y1 is unused
  assert(type != OpaquePredicate::PredicateIndeterminate &&
         "Formula 1 does not support indeterminate!");

  // 96 bit integer type
  Type *intType = (Type *)Type::getIntNTy(block->getContext(), 96);

  // Sign extend to 96 bits
  Value *x1 = (Value *)new SExtInst(x, intType, "", block);

  // x^2
  Value *x2 =
      (Value *)BinaryOperator::Create(Instruction::Mul, x1, x1, "", block);

  // x^3
  Value *x3 =
      (Value *)BinaryOperator::Create(Instruction::Mul, x2, x1, "", block);

  // x^3 - x
  Value *x4 =
      (Value *)BinaryOperator::Create(Instruction::Sub, x3, x1, "", block);

  Value *three = ConstantInt::get(intType, 3, false);

  // Finally the mod
  Value *mod =
      (Value *)BinaryOperator::Create(Instruction::SRem, x4, three, "", block);

  Value *zero = ConstantInt::get(intType, 0, false);

  Value *condition;
  // Compare
  if (type == OpaquePredicate::PredicateTrue)
    condition = CmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_EQ, mod, zero,
                                "", block);
  else
    condition = CmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_NE, mod, zero,
                                "", block);

  return condition;
}

// x % 2 == 0 || (x^2 - 1) % 8 == 0 for all x in Z
Value *OpaquePredicate::formula2(BasicBlock *block, Value *x, Value *y1,
                                 OpaquePredicate::PredicateType type) {
  // y1 is unused
  assert(type != OpaquePredicate::PredicateIndeterminate &&
         "Formula 2 does not support indeterminate!");

  // 64 bit integer type
  Type *intType = (Type *)Type::getIntNTy(block->getContext(), 64);

  Value *zero = ConstantInt::get(intType, 0, false);
  Value *one = ConstantInt::get(intType, 1, false);
  Value *two = ConstantInt::get(intType, 2, false);
  Value *eight = ConstantInt::get(intType, 8, false);

  // Sign extend to 64 bits
  Value *x1 = (Value *)new SExtInst(x, intType, "", block);

  // x^2
  Value *x2 =
      (Value *)BinaryOperator::Create(Instruction::Mul, x1, x1, "", block);
  // x^2- 1
  Value *x3 =
      (Value *)BinaryOperator::Create(Instruction::Sub, x2, one, "", block);

  // x % 2
  Value *xMod2 =
      (Value *)BinaryOperator::Create(Instruction::SRem, x1, two, "", block);

  // (x^2 - 1) mod 8
  Value *xMod8 =
      (Value *)BinaryOperator::Create(Instruction::SRem, x3, eight, "", block);

  // x % 2 == 0
  Value *lhs = CmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_EQ, xMod2,
                               zero, "", block);
  // (x^2 - 1) mod 8 == 0
  Value *rhs = CmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_EQ, xMod8,
                               zero, "", block);

  Value *condition =
      (Value *)BinaryOperator::Create(Instruction::Or, lhs, rhs, "", block);

  // If false, we just negate
  if (type == OpaquePredicate::PredicateTrue)
    return condition;
  else
    return BinaryOperator::CreateNot(condition, "", block);
}

OpaquePredicate::Formula
OpaquePredicate::getFormula(OpaquePredicate::Randomner randomner) {
  static const int number = 3;
  static Formula formales[number] = { formula0, formula1, formula2 };
  int n = randomner() % number;
  DEBUG(errs() << "[Opaque Predicate] Formula " << n << "\n");
  return formales[n];
}

std::vector<GlobalVariable *> OpaquePredicate::prepareModule(Module &M) {
  assert(opaqueGlobal >= 2 &&
         "Opaque Predicates need at least 2 global variables");
  DEBUG(errs() << "[Opaque Predicate] Creating " << opaqueGlobal
               << " globals\n");
  std::vector<GlobalVariable *> globals(opaqueGlobal);
  for (unsigned i = 0; i < opaqueGlobal; ++i) {
    Twine globalName("");
    DEBUG(globalName = globalName.concat(Twine("global_")).concat(Twine(i)));
    Value *zero = ConstantInt::get(Type::getInt32Ty(M.getContext()), 0, true);
    GlobalVariable *global = new GlobalVariable(
        M, Type::getInt32Ty(M.getContext()), false, GlobalValue::CommonLinkage,
        (Constant *)zero, globalName);
    assert(global && "Null globals created!");
    globals[i] = global;
  }
  return globals;
}

OpaquePredicate::PredicateType
OpaquePredicate::create(BasicBlock *headBlock, BasicBlock *trueBlock,
                        BasicBlock *falseBlock,
                        const std::vector<GlobalVariable *> &globals,
                        Randomner randomner, PredicateTypeRandomner typeRand) {

  PredicateType type = typeRand();
  switch (type) {
  case PredicateFalse:
    createFalse(headBlock, trueBlock, falseBlock, globals, randomner);
    break;
  case PredicateTrue:
    createTrue(headBlock, trueBlock, falseBlock, globals, randomner);
  case PredicateIndeterminate:
    break;
  default:
    llvm_unreachable("Unknown type of predicate");
  }
  return type;
}

void OpaquePredicate::createTrue(BasicBlock *headBlock, BasicBlock *trueBlock,
                                 BasicBlock *falseBlock,
                                 const std::vector<GlobalVariable *> &globals,
                                 OpaquePredicate::Randomner randomner) {
  // Get our x and y
  GlobalVariable *x = globals[randomner() % globals.size()];
  GlobalVariable *y = globals[randomner() % globals.size()];

  while (x == y) {
    y = globals[randomner() % globals.size()];
  }

  // Advance our x and y
  Value *x1 = advanceGlobal(headBlock, x, randomner);
  Value *y1 = advanceGlobal(headBlock, y, randomner);

  Formula formula = getFormula(randomner);
  Value *condition = formula(headBlock, x1, y1, PredicateTrue);

  // Branch
  BranchInst::Create(trueBlock, falseBlock, condition, headBlock);
}

void OpaquePredicate::createFalse(BasicBlock *headBlock, BasicBlock *trueBlock,
                                  BasicBlock *falseBlock,
                                  const std::vector<GlobalVariable *> &globals,
                                  OpaquePredicate::Randomner randomner) {
  // Get our x and y
  GlobalVariable *x = globals[abs(randomner()) % globals.size()];
  GlobalVariable *y = globals[abs(randomner()) % globals.size()];

  while (x == y) {
    y = globals[randomner() % globals.size()];
  }

  // Advance our x and y
  Value *x1 = advanceGlobal(headBlock, x, randomner);
  Value *y1 = advanceGlobal(headBlock, y, randomner);

  Formula formula = getFormula(randomner);
  Value *condition = formula(headBlock, x1, y1, PredicateFalse);

  // Branch
  BranchInst::Create(trueBlock, falseBlock, condition, headBlock);
}

void OpaquePredicate::createStub(BasicBlock *block, BasicBlock *trueBlock,
                                 BasicBlock *falseBlock,
                                 OpaquePredicate::PredicateType type,
                                 bool markUnreachable) {
  // Check if basic block has a terminator, if so, remove it
  if (block->getTerminator()) {
    block->getTerminator()->eraseFromParent();
  }

  Value *lhs = ConstantFP::get(Type::getFloatTy(block->getContext()), 1.0);
  Value *rhs = ConstantFP::get(Type::getFloatTy(block->getContext()), 1.0);
  FCmpInst *condition = new FCmpInst(*block, FCmpInst::FCMP_TRUE, lhs, rhs);

  // Bogus conditional branch
  BranchInst *branch =
      BranchInst::Create(trueBlock, falseBlock, (Value *)condition, block);
  tagInstruction(*branch, stubName, type);

  if (!markUnreachable) {
    LLVMContext &context = block->getContext();
    unsigned metaKind = context.getMDKindID(unreachableMarkName);
    MDNode *metaNode =
        MDNode::get(context, MDString::get(context, unreachableMarkName));
    branch->setMetadata(metaKind, metaNode);
  }
}

void OpaquePredicate::tagInstruction(Instruction &inst, StringRef metaKindName,
                                     OpaquePredicate::PredicateType type) {
  LLVMContext &context = inst.getContext();
  unsigned metaKind = context.getMDKindID(metaKindName);
  MDNode *metaNode =
      MDNode::get(context, MDString::get(context, getStringRef(type)));
  inst.setMetadata(metaKind, metaNode);
}

OpaquePredicate::PredicateType
OpaquePredicate::getInstructionType(Instruction &inst, StringRef metaKindName) {
  LLVMContext &context = inst.getContext();
  unsigned metaKind = context.getMDKindID(metaKindName);

  MDNode *meta = inst.getMetadata(metaKind);

  if (!meta) {
    return PredicateNone;
  }

  // Checks
  assert(meta->getNumOperands() == 1 && "Meta node should only have 1 operand");

  MDString *mdstring = dyn_cast<MDString>(meta->getOperand(0));
  assert(mdstring && "MDNode Value should be of type MDString!");

  StringRef str = mdstring->getString();
  if (str == getStringRef(PredicateFalse)) {
    return PredicateFalse;
  } else if (str == getStringRef(PredicateTrue)) {
    return PredicateTrue;
  } else if (str == getStringRef(PredicateIndeterminate)) {
    return PredicateIndeterminate;
  } else if (str == getStringRef(PredicateRandom)) {
    return PredicateRandom;
  } else {
    llvm_unreachable("Unknown type");
  }
}

bool OpaquePredicate::isBasicBlockUnreachable(BasicBlock &block) {
  Instruction &inst = *(block.begin());
  PredicateType type = getInstructionType(inst, unreachableName);
  if (type == PredicateNone)
    return false;
  else {
    // Let's do some checks
    BasicBlock *predecessor = block.getSinglePredecessor();
    assert(predecessor && "Unreachable block should only have 1 predecessor");

    BranchInst *branch = dyn_cast<BranchInst>(predecessor->getTerminator());
    assert(branch && "Predecessor block should have a BranchInst terminator");
    assert(branch->isConditional() &&
           "Branch in predecessor should be conditional");
    // Checks conditions
    if (type == PredicateTrue) {
      // Then this block should be the "false" branch
      assert(branch->getSuccessor(1) == &block &&
             "Unreachable block is the wrong successor of its predecessor");
    } else if (type == PredicateFalse) {
      // Then this block should be the "true" branch
      assert(branch->getSuccessor(0) == &block &&
             "Unreachable block is the wrong successor of its predecessor");
    } else {
      llvm_unreachable("Unsupported predicate type");
    }

    return true;
  }
}
void OpaquePredicate::clearUnreachable(BasicBlock &block) {
  Instruction &inst = *(block.begin());
  LLVMContext &context = inst.getContext();
  unsigned metaKind = context.getMDKindID(unreachableName);
  inst.setMetadata(metaKind, nullptr);
}

raw_ostream &operator<<(raw_ostream &stream,
                        const OpaquePredicate::PredicateType &o) {
  switch (o) {
  case OpaquePredicate::PredicateFalse:
    stream << "False";
    break;
  case OpaquePredicate::PredicateTrue:
    stream << "True";
    break;
  case OpaquePredicate::PredicateIndeterminate:
    stream << "Indeterminate";
    break;
  case OpaquePredicate::PredicateRandom:
    stream << "Random";
    break;
  default:
    llvm_unreachable("Unknown type of predicate");
  }

  return stream;
}

// c.f. http://llvm.org/docs/doxygen/html/DebugInfo_8cpp_source.html
void OpaquePredicate::cleanDebug(BasicBlock &block) {
  Module *module = block.getParent()->getParent();
  Function *declare = module->getFunction("llvm.dbg.declare");
  Function *value = module->getFunction("llvm.dbg.value");

  std::vector<Instruction *> removeInstructions;

  for (auto &inst : block) {
    if (CallInst *call = dyn_cast<CallInst>(&inst)) {
      Function *called = call->getCalledFunction();
      if (called == declare || called == value) {
        removeInstructions.push_back(&inst);
      }
    }

    if (inst.getDebugLoc().isUnknown()) {
      inst.setDebugLoc(DebugLoc());
    }
    SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
    inst.getAllMetadataOtherThanDebugLoc(MDs);
    for (const auto &MD : MDs) {
      inst.setMetadata(MD.first, nullptr);
    }
  }

  for (Instruction *inst : removeInstructions) {
    inst->eraseFromParent();
  }
}

StringRef OpaquePredicate::stubName("opaque_stub");
StringRef OpaquePredicate::unreachableName("opaque_unreachable");
StringRef OpaquePredicate::unreachableMarkName("opaque_mark");
char OpaquePredicate::ID = 0;
static RegisterPass<OpaquePredicate>
    X("opaque-predicate", "Replace stub branch with opaque predicates", false,
      false);
