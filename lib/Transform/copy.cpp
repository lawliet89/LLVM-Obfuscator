//=== copy.cpp - Copy function pass======   -------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "copy"
#include "Transform/copy.h"
#include "Transform/obf_utilities.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"
#include <algorithm>
#include <vector>
#include <chrono>

// TODO: Synergise other passes with copying

static cl::list<std::string> copyFunc("copyFunc", cl::CommaSeparated,
                                      cl::desc("Only copy some functions: "
                                               "copyFunc=\"func1,func2\""));

static cl::opt<double> copyProbability(
    "copyProbability", cl::init(0.2),
    cl::desc(
        "Probability that a function will be copied if list is not specified"));

static cl::opt<std::string>
copySeed("copySeed", cl::init(""),
         cl::desc("Seed for random number generator. Defaults to system time"));

bool Copy::runOnModule(Module &M) {
  // Initialise
  if (copyProbability < 0.f || copyProbability > 1.f) {
    LLVMContext &ctx = getGlobalContext();
    ctx.emitError("Copy: Probability must be between 0 and 1");
  }

  if (copyProbability == 0.f) {
    return false;
  }
  // Seed engine and create distribution
  if (!copySeed.empty()) {
    std::seed_seq seed(copySeed.begin(), copySeed.end());
    engine.seed(seed);
  } else {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    engine.seed(seed);
  }

  trial.param(std::bernoulli_distribution::param_type((double)copyProbability));

  bool hasBeenModified = false;
  auto funcListStart = copyFunc.begin(), funcListEnd = copyFunc.end();
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;

    DEBUG(errs() << "Copy: Function '" << F.getName() << "'\n");
    if (copyFunc.empty()) {
      // Play dice
      if (!trial(engine)) {
        DEBUG(errs() << "\t\tSkipping: Bernoulli trial failed\n");
        continue;
      }
    } else {
      if (std::find(funcListStart, funcListEnd, F.getName()) == funcListEnd) {
          DEBUG(errs() << "\tFunction not requested -- skipping\n");
          continue;
        }
    }
  }

  return hasBeenModified;
}

char Copy::ID = 0;
static RegisterPass<Copy>
X("copy", "Copy function pass", false, false);
