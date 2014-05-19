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

#ifndef OPAQUE_PREDICATE_H
#define OPAQUE_PREDICATE_H
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include <functional>
#include <vector>
using namespace llvm;

struct OpaquePredicate : public ModulePass {
  enum PredicateType {
    PredicateFalse = 0x0,
    PredicateTrue = 0x1,
    PredicateIndeterminate = 0x2,
    PredicateRandom = 0x3,
    PredicateNone = 0xFF
  };

  typedef std::function<Value *(BasicBlock *, Value *, Value *,
                                OpaquePredicate::PredicateType)> Formula;
  typedef std::function<int()> Randomner;
  typedef std::function<PredicateType()> PredicateTypeRandomner;

  static char ID;
  std::mt19937_64 engine;
  static StringRef stubName;
  static StringRef unreachableMarkName;
  static StringRef unreachableName;

  OpaquePredicate() : ModulePass(ID) {}
  virtual bool runOnModule(Module &M);

  static void createStub(BasicBlock *block, BasicBlock *trueBlock,
                         BasicBlock *falseBlock,
                         PredicateType type = PredicateRandom,
                         bool markUnreachable = true);

  static bool isBasicBlockUnreachable(BasicBlock &block);
  static void clearUnreachable(BasicBlock &block);

private:
  // Prepare module for opaque predicates by adding global variables to the
  // module
  // Returns a vector of pointers to the global variables generated
  // Needs at least 2 global variables
  static std::vector<GlobalVariable *> prepareModule(Module &M);

  // Given a BasicBlock with NO terminator, and two successor blocks
  // Generate a randomly selected opaque predicate to replace the terminator
  // and then branch to the given blocks
  // Returns the type of predicate produced
  static PredicateType create(BasicBlock *headBlock, BasicBlock *trueBlock,
                              BasicBlock *falseBlock,
                              const std::vector<GlobalVariable *> &globals,
                              Randomner randomner,
                              PredicateTypeRandomner typeRand);

  // Given a BasicBlock with NO terminator, and two successor blocks
  // Generate an always true opaque predicate to replace the terminator
  // and then branch to the given blocks
  // Returns the type of predicate produced
  static void createTrue(BasicBlock *headBlock, BasicBlock *trueBlock,
                         BasicBlock *falseBlock,
                         const std::vector<GlobalVariable *> &globals,
                         Randomner randomner);

  // Given a BasicBlock with NO terminator, and two successor blocks
  // Generate an always false opaque predicate to replace the terminator
  // and then branch to the given blocks
  // Returns the type of predicate produced
  static void createFalse(BasicBlock *headBlock, BasicBlock *trueBlock,
                          BasicBlock *falseBlock,
                          const std::vector<GlobalVariable *> &globals,
                          Randomner randomner);

  static Value *formula0(BasicBlock *block, Value *x1, Value *y1,
                         OpaquePredicate::PredicateType type);

  static Value *formula1(BasicBlock *block, Value *x1, Value *y1,
                         OpaquePredicate::PredicateType type);

  static Value *formula2(BasicBlock *block, Value *x1, Value *y1,
                         OpaquePredicate::PredicateType type);

  static Formula getFormula(OpaquePredicate::Randomner randomner);

  static Value *advanceGlobal(BasicBlock *block, GlobalVariable *global,
                              OpaquePredicate::Randomner randomner);

  static StringRef getStringRef(PredicateType type) {
    switch (type) {
    case PredicateFalse:
      return StringRef("false");
    case PredicateTrue:
      return StringRef("true");
    case PredicateIndeterminate:
      return StringRef("indeterminate");
    case PredicateRandom:
      return StringRef("random");
    default:
      llvm_unreachable("Unknown type");
    }
  }
  static void tagInstruction(Instruction &inst, StringRef metaKindName,
                             PredicateType type = PredicateRandom);
  static PredicateType getInstructionType(Instruction &inst,
                                          StringRef metaKindName);

  static void cleanDebug(BasicBlock &block);
};

// Overloads for PredicateType
raw_ostream &operator<<(raw_ostream &stream,
                        const OpaquePredicate::PredicateType &o);

#endif // OPAQUE_PREDICATE_H
