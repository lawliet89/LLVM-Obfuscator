//=== inline_function.cpp - Inline function pass======   ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "inline_function"
#include "Transform/inline_function.h"
#include "Transform/obf_utilities.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CFG.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <algorithm>
#include <vector>
#include <chrono>

static cl::opt<double> inlineProbability(
    "inlineProbability", cl::init(0.2),
    cl::desc("Probability that a function call will be inlined"));

static cl::opt<std::string> inlineSeed(
    "inlineSeed", cl::init(""),
    cl::desc("Seed for random number generator. Defaults to system time"));

static cl::opt<unsigned> inlinePass(
    "inlinePass", cl::init(2),
    cl::desc("Number of passes to attempt to inline function calls"));

static cl::opt<bool>
    disableInline("disableInline", cl::init(false),
                  cl::desc("Disable Inline function pass regardless. Useful "
                           "when used in -OX mode."));

bool InlineFunctionPass::doInitialization(Module &M) {
  if (inlineProbability < 0.f || inlineProbability > 1.f) {
    LLVMContext &ctx = getGlobalContext();
    ctx.emitError("InlineFunctionPass: Probability must be between 0 and 1");
  }

  // Seed engine and create distribution
  if (!inlineSeed.empty()) {
    std::seed_seq seed(inlineSeed.begin(), inlineSeed.end());
    engine.seed(seed);
  } else {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    engine.seed(seed);
  }

  trial.param(
      std::bernoulli_distribution::param_type((double)inlineProbability));

  return false;
}

bool InlineFunctionPass::runOnFunction(Function &F) {
  if (disableInline)
    return false;
  // If the function is declared elsewhere in other translation unit
  // we should not modify it here
  if (F.isDeclaration()) {
    return false;
  }
  if (inlineProbability == 0.f) {
    return false;
  }

  bool hasBeenModified = false;
  DEBUG(errs() << "InlineFunctionPass: Function '" << F.getName() << "'\n");

  for (unsigned i = 0; i < inlinePass; ++i) {
    DEBUG(errs() << "\tPass " << i << ":\n");
    DEBUG(errs() << "\t\tGathering CallSites\n");

    std::vector<CallSite> callsites;
    for (auto &block : F) {
      for (auto &inst : block) {
        if (InvokeInst *invoke = dyn_cast<InvokeInst>(&inst)) {
          if (invoke->getCalledFunction()->isIntrinsic())
            continue;
          DEBUG(errs() << "\t\t\t" << inst << "\n");
          callsites.push_back(CallSite(invoke));
        } else if (CallInst *call = dyn_cast<CallInst>(&inst)) {
          if (call->getCalledFunction()->isIntrinsic())
            continue;
          DEBUG(errs() << "\t\t\t" << inst << "\n");
          callsites.push_back(CallSite(call));
        }
      }
    }
    if (callsites.empty()) {
      DEBUG(errs() << "\tZero CallSites -- skipping function\n");
      return hasBeenModified;
    }

    for (CallSite callsite : callsites) {
      DEBUG(errs() << "\t\t" << *(callsite.getInstruction()) << "\n");
      if (!trial(engine)) {
        DEBUG(errs() << "\t\t\tSkipping: Bernoulli trial failed\n");
        continue;
      }
      InlineFunctionInfo IFI;
      bool inlined = InlineFunction(callsite, IFI);

      if (inlined) {
        hasBeenModified |= true;
        DEBUG(errs() << "\t\t\tFunction call inlined\n");
      } else {
        DEBUG(errs() << "\t\t\tIneligible for inling\n");
      }
    }
  }

  return hasBeenModified;
}

char InlineFunctionPass::ID = 0;
static RegisterPass<InlineFunctionPass> X("inline-function",
                                          "Inline function obfuscation pass",
                                          false, false);
