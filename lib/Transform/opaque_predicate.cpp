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

#include "Transform/opaque_predicate.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include <cassert>
using namespace llvm;

namespace {
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
  Value *random = ConstantInt::get(Type::getInt32Ty(block->getContext()),
                                   randomner(), true);
  LoadInst *load = new LoadInst((Value *)global, "", block);
  BinaryOperator *add = BinaryOperator::Create(Instruction::Add, (Value *)load,
                                               (Value *)random, "", block);
  // Store
  StoreInst *store = new StoreInst(add, (Value *)global, block);
  return (Value *)add;
}

// 7y^2 -1 != x^2 for all x, y in Z
Value *formula1(BasicBlock *block, Value *x1, Value *y1, Value *&x2, Value *&y4,
                OpaquePredicate::PredicateType type) {

  assert(type != OpaquePredicate::PredicateIndeterminate &&
         "Formula 1 does not support indeterminate!");

  Value *seven =
      ConstantInt::get(Type::getInt32Ty(block->getContext()), 7, false);
  Value *one =
      ConstantInt::get(Type::getInt32Ty(block->getContext()), 1, false);
  // x^2
  x2 = (Value *)BinaryOperator::Create(Instruction::Mul, x1, x1, "", block);
  // y^2
  Value *y2 =
      (Value *)BinaryOperator::Create(Instruction::Mul, y1, y1, "", block);
  // 7y^2
  Value *y3 =
      (Value *)BinaryOperator::Create(Instruction::Mul, y2, seven, "", block);
  // 7y^2 - 1
  y4 = (Value *)BinaryOperator::Create(Instruction::Sub, y3, one, "", block);

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
};

namespace OpaquePredicate {
std::vector<GlobalVariable *> prepareModule(Module &M, unsigned number) {
  assert(number >= 2 && "Opaque Predicates need at least 2 global variables");
  std::vector<GlobalVariable *> globals(number);
  for (unsigned i = 0; i < number; ++i) {
    Value *zero = ConstantInt::get(Type::getInt32Ty(M.getContext()), 0, true);
    GlobalVariable *global =
        new GlobalVariable(M, Type::getInt32Ty(M.getContext()), false,
                           GlobalValue::CommonLinkage, (Constant *)zero);
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
  GlobalVariable *x = globals[abs(randomner()) % globals.size()];
  GlobalVariable *y = globals[abs(randomner()) % globals.size()];

  // Advance our x and y
  Value *x1 = advanceGlobal(headBlock, x, randomner);
  Value *y1 = advanceGlobal(headBlock, y, randomner);

  Value *x2, *y2;
  Value *condition = formula1(headBlock, x1, y1, x2, y2, PredicateTrue);

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

  Value *x2, *y2;
  Value *condition = formula1(headBlock, x1, y1, x2, y2, PredicateFalse);

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
