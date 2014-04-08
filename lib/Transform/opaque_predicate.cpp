//=== opaque_predicate.h - Manufactures opaque predicates -----------------===//
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
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
using namespace llvm;

namespace {

typedef std::function<Value *(BasicBlock *, Value *, Value *,
                              OpaquePredicate::PredicateType)> Formula;

void check(BasicBlock *headBlock) {
  // Check that our head block has a unconditional branch
  TerminatorInst *terminator = headBlock->getTerminator();
  BranchInst *branch = dyn_cast<BranchInst>(terminator);
  assert(branch && "Terminator instruction must be a branch");
  assert(branch->isUnconditional() &&
         "Branch instruction must be unconditional");
}

// TODO: Use some runtime randomniser? Maybe?
Value *advanceGlobal(BasicBlock *block, GlobalVariable *global,
                     OpaquePredicate::Randomner randomner) {
  assert(global && "Null global pointer");
  DEBUG(errs() << "[Opaque Predicate] Randomly advancing global\n");
  DEBUG(errs() << "\tCreating Random Constant\n");
  Value *random = ConstantInt::get(Type::getInt32Ty(block->getContext()),
                                   randomner(), true);
  DEBUG(errs() << "\tLoading global\n");
  LoadInst *load = new LoadInst((Value *)global, "", block);
  DEBUG(errs() << "\tAdding global\n");
  BinaryOperator *add = BinaryOperator::Create(Instruction::Add, (Value *)load,
                                               (Value *)random, "", block);
  DEBUG(errs() << "\tStoring global\n");
  new StoreInst(add, (Value *)global, block);
  return (Value *)add;
}

// 7y^2 -1 != x^2 for all x, y in Z
Value *formula0(BasicBlock *block, Value *x1, Value *y1,
                OpaquePredicate::PredicateType type) {

  assert(type != OpaquePredicate::PredicateIndeterminate &&
         "Formula 0 does not support indeterminate!");

  Value *seven =
      ConstantInt::get(Type::getInt32Ty(block->getContext()), 7, false);
  Value *one =
      ConstantInt::get(Type::getInt32Ty(block->getContext()), 1, false);
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
Value *formula1(BasicBlock *block, Value *x1, Value *y1,
                OpaquePredicate::PredicateType type) {
  // y1 is unused
  assert(type != OpaquePredicate::PredicateIndeterminate &&
         "Formula 1 does not support indeterminate!");

  // x^2
  Value *x2 =
      (Value *)BinaryOperator::Create(Instruction::Mul, x1, x1, "", block);

  // x^3
  Value *x3 =
      (Value *)BinaryOperator::Create(Instruction::Mul, x2, x1, "", block);

  // x^3 - x
  Value *x4 =
      (Value *)BinaryOperator::Create(Instruction::Sub, x3, x1, "", block);

  Value *three =
      ConstantInt::get(Type::getInt32Ty(block->getContext()), 3, false);

  // Finally the mod
  Value *mod =
      (Value *)BinaryOperator::Create(Instruction::SRem, x4, three, "", block);

  Value *zero =
      ConstantInt::get(Type::getInt32Ty(block->getContext()), 0, true);

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
Value *formula2(BasicBlock *block, Value *x1, Value *y1,
                OpaquePredicate::PredicateType type) {
  // y1 is unused
  assert(type != OpaquePredicate::PredicateIndeterminate &&
         "Formula 2 does not support indeterminate!");

  Value *zero =
      ConstantInt::get(Type::getInt32Ty(block->getContext()), 0, true);
  Value *one =
      ConstantInt::get(Type::getInt32Ty(block->getContext()), 1, false);
  Value *two =
      ConstantInt::get(Type::getInt32Ty(block->getContext()), 2, false);
  Value *eight =
      ConstantInt::get(Type::getInt32Ty(block->getContext()), 8, false);

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
  else if (type == OpaquePredicate::PredicateFalse)
    return BinaryOperator::CreateNot(condition, "", block);
}

Formula getFormula(OpaquePredicate::Randomner randomner) {
  static const int number = 3;
  static Formula formales[number] = { formula0, formula1, formula2 };
  int n = randomner() % number;
  DEBUG(errs() << "[Opaque Predicate] Formula " << n << "\n");
  return formales[n];
}
};

namespace OpaquePredicate {
std::vector<GlobalVariable *> prepareModule(Module &M, unsigned number) {
  assert(number >= 2 && "Opaque Predicates need at least 2 global variables");
  DEBUG(errs() << "[Opaque Predicate] Creating " << number << " globals\n");
  std::vector<GlobalVariable *> globals(number);
  for (unsigned i = 0; i < number; ++i) {
    Value *zero = ConstantInt::get(Type::getInt32Ty(M.getContext()), 0, true);
    GlobalVariable *global = new GlobalVariable(
        M, Type::getInt32Ty(M.getContext()), false, GlobalValue::CommonLinkage,
        (Constant *)zero, "", nullptr, GlobalVariable::GeneralDynamicTLSModel);
    assert(global && "Null globals created!");
    globals[i] = global;
  }
  return globals;
}

PredicateType create(BasicBlock *headBlock, BasicBlock *trueBlock,
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

void createTrue(BasicBlock *headBlock, BasicBlock *trueBlock,
                BasicBlock *falseBlock,
                const std::vector<GlobalVariable *> &globals,
                Randomner randomner) {

  // Check that the basic block is in a form that we want
  check(headBlock);
  headBlock->getTerminator()->eraseFromParent();

  // Get our x and y
  GlobalVariable *x = globals[randomner() % globals.size()];
  GlobalVariable *y = globals[randomner() % globals.size()];

  // Advance our x and y
  Value *x1 = advanceGlobal(headBlock, x, randomner);
  Value *y1 = advanceGlobal(headBlock, y, randomner);

  Formula formula = getFormula(randomner);
  Value *condition = formula(headBlock, x1, y1, PredicateTrue);

  // Branch
  BranchInst::Create(trueBlock, falseBlock, condition, headBlock);
}

void createFalse(BasicBlock *headBlock, BasicBlock *trueBlock,
                 BasicBlock *falseBlock,
                 const std::vector<GlobalVariable *> &globals,
                 Randomner randomner) {

  // Check that the basic block is in a form that we want
  check(headBlock);
  headBlock->getTerminator()->eraseFromParent();

  // Get our x and y
  GlobalVariable *x = globals[abs(randomner()) % globals.size()];
  GlobalVariable *y = globals[abs(randomner()) % globals.size()];

  // Advance our x and y
  Value *x1 = advanceGlobal(headBlock, x, randomner);
  Value *y1 = advanceGlobal(headBlock, y, randomner);

  Formula formula = getFormula(randomner);
  Value *condition = formula(headBlock, x1, y1, PredicateFalse);

  // Branch
  BranchInst::Create(trueBlock, falseBlock, condition, headBlock);
}
};

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
  default:
    llvm_unreachable("Unknown type of predicate");
  }

  return stream;
}
