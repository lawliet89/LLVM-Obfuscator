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

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include <functional>
#include <vector>
using namespace llvm;

namespace OpaquePredicate {
enum PredicateType {
  PredicateFalse = 0x0,
  PredicateTrue = 0x1,
  PredicateIndeterminate = 0x2
};

typedef std::function<int()> Randomner;
typedef std::function<PredicateType()> PredicateTypeRandomner;

// Prepare module for opaque predicates by adding global variables to the module
// Returns a vector of pointers to the global variables generated
// Needs at least 2 global variables
std::vector<GlobalVariable *> prepareModule(Module &M, unsigned number = 4);

// Given a BasicBlock with an unconditional terminator, and two successor blocks
// Generate a randomly selected opaque predicate to replace the terminator
// and then branch to the given blocks
// Returns the type of predicate produced
PredicateType create(BasicBlock *headBlock, BasicBlock *trueBlock,
                     BasicBlock *falseBlock,
                     const std::vector<GlobalVariable *> &globals,
                     Randomner randomner, PredicateTypeRandomner typeRand);

// Given a BasicBlock with an unconditional terminator, and two successor blocks
// Generate an always true opaque predicate to replace the terminator
// and then branch to the given blocks
// Returns the type of predicate produced
void createTrue(BasicBlock *headBlock, BasicBlock *trueBlock,
                BasicBlock *falseBlock,
                const std::vector<GlobalVariable *> &globals,
                Randomner randomner);

// Given a BasicBlock with an unconditional terminator, and two successor blocks
// Generate an always false opaque predicate to replace the terminator
// and then branch to the given blocks
// Returns the type of predicate produced
void createFalse(BasicBlock *headBlock, BasicBlock *trueBlock,
                 BasicBlock *falseBlock,
                 const std::vector<GlobalVariable *> &globals,
                 Randomner randomner);
};

// Overloads for PredicateType
raw_ostream &operator<<(raw_ostream &stream,
                        const OpaquePredicate::PredicateType &o);

#endif // OPAQUE_PREDICATE_H
