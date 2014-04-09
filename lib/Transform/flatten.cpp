//=== flatten.cpp - Flatten Control Flow Pass  ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// http://ac.inf.elte.hu/Vol_030_2009/003.pdf
#define DEBUG_TYPE "flatten"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/User.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"
#include <algorithm>
#include <vector>
#include <chrono>
#include <random>

using namespace llvm;

static cl::list<std::string>
    flattenFunc("flattenFunc", cl::CommaSeparated,
                cl::desc("Insert Bogus Control Flow only for some functions: "
                         "flattenFunc=\"func1,func2\""));

static cl::opt<std::string> flattenSeed(
    "flattenSeed", cl::init(""),
    cl::desc("Seed for random number generator. Defaults to system time"));

namespace {
struct Flatten : public FunctionPass {
  static char ID;
  std::minstd_rand engine;

  Flatten() : FunctionPass(ID) {}

  // Initialise and check options
  virtual bool doInitialization(Module &M) {
    // Seed engine and create distribution
    if (!flattenSeed.empty()) {
      std::seed_seq seed(flattenSeed.begin(), flattenSeed.end());
      engine.seed(seed);
    } else {
      unsigned seed =
          std::chrono::system_clock::now().time_since_epoch().count();
      engine.seed(seed);
    }

    return false;
  }

  virtual bool runOnFunction(Function &F) {
    // If the function is declared elsewhere in other translation unit
    // we should not modify it here
    if (F.isDeclaration()) {
      return false;
    }
    DEBUG(errs() << "flatten: Function '" << F.getName() << "'\n");

    // Check if function is requested
    auto funcListStart = flattenFunc.begin(), funcListEnd = flattenFunc.end();
    if (flattenFunc.size() != 0 &&
        std::find(funcListStart, funcListEnd, F.getName()) == funcListEnd) {
      DEBUG(errs() << "\tFunction not requested -- skipping\n");
      return false;
    }

    // Use a vector to store the list of blocks
    std::vector<BasicBlock *> blocks;
    blocks.reserve(F.size());

    DEBUG(errs() << "\t" << F.size() << " basic blocks found\n");
    Twine blockPrefix = "block_";
    unsigned i = 0;
    DEBUG(errs() << "\tListing and filtering blocks\n");
    // Get original list of blocks
    for (auto &block : F) {
      DEBUG(if (!block.hasName()) {
        block.setName(blockPrefix + Twine(i++));
      });

      DEBUG(errs() << "\tBlock " << block.getName() << "\n");
      BasicBlock::iterator inst1 = block.begin();
      if (block.getFirstNonPHIOrDbgOrLifetime()) {
        inst1 = block.getFirstNonPHIOrDbgOrLifetime();
      }
      if (block.isLandingPad()) {
        DEBUG(errs() << "\t\tSkipping: Landing pad block\n");
        continue;
      }
      if (&block == &F.getEntryBlock()){
        DEBUG(errs() << "\t\tSkipping: Entry block\n");
        continue;
      }

      DEBUG(errs() << "\t\tAdding block\n");
      blocks.push_back(&block);
    }

    DEBUG(errs() << "\t" << blocks.size() << " basic blocks remaining\n");
    if (blocks.size() < 2) {
      DEBUG(errs() << "\tNothing left to flatten\n");
      return false;
    }
    // Setup other variables
    BasicBlock &entryBlock = F.getEntryBlock();

    if (entryBlock.getTerminator()->getNumSuccessors() == blocks.size()) {
      DEBUG(errs() << "\tFunction is trivial -- already flat control flow\n");
      return false;
    }

    DEBUG_WITH_TYPE("cfg", F.viewCFG());

    return true;
  }

  // Finalisation will add the necessary opaque predicates
  // virtual bool doFinalization(Module &M) { return false; }
  // virtual void getAnalysisUsage(AnalysisUsage &Info) const {}
};
}

char Flatten::ID = 0;
static RegisterPass<Flatten> X("flatten", "Flatten function control flow",
                               false, false);

  // http://homes.cs.washington.edu/~bholt/posts/llvm-quick-tricks.html
static RegisterStandardPasses Y(PassManagerBuilder::EP_OptimizerLast,
                                [](const PassManagerBuilder &,
                                   PassManagerBase &PM) {
  PM.add(new Flatten());
});
