//=== schedule.cpp - Schedule the passes------ ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "Transform/boguscf.h"
#include "Transform/cleanup.h"
#include "Transform/copy.h"
#include "Transform/flatten.h"
#include "Transform/identifier_renamer.h"
#include "Transform/inline_function.h"
#include "Transform/loop_boguscf.h"
#include "Transform/opaque_predicate.h"
#include "Transform/replace_instruction.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/CommandLine.h"
#include <vector>

using namespace llvm;

static cl::opt<bool>
    noObfSchedule("noObfSchedule", cl::init(false),
                  cl::desc("Do not schedule Obfuscation passes"));

static cl::opt<bool> trivialObfuscation(
    "trivialObfuscation", cl::init(false),
    cl::desc(
        "Only scheudle trivial obfuscation passes that do not modify the CFG"));

enum ScheduleOptions {
  copyPass,
  inlineFunctionPass,
  bogusCFPass,
  loopBCFPass,
  opaquePredicatePass,
  replaceInstructionPass,
  flattenPass,
  cleanupPass,
  identifierRenamerPass
};

cl::list<ScheduleOptions> ObfuscationList(
    cl::desc("Available Obfuscations for scheduling:"),
    cl::values(clEnumVal(copyPass, "Copy Function"),
               clEnumVal(inlineFunctionPass, "Inline function calls"),
               clEnumVal(bogusCFPass, "Insert bogus control flow"),
               clEnumVal(loopBCFPass, "Loop bogus control flow"),
               clEnumVal(opaquePredicatePass, "Create opaque predicates"),
               clEnumVal(replaceInstructionPass, "Replace Instruction"),
               clEnumVal(flattenPass, "Flatten cotnrol flow"),
               clEnumVal(cleanupPass, "Cleanup"),
               clEnumVal(identifierRenamerPass, "Rename identifiers"),
               clEnumValEnd));

namespace {

std::vector<Pass *> getPasses() {
  std::vector<Pass *> passes;

  if (trivialObfuscation) {
    passes.push_back(new Copy());
    passes.push_back(new InlineFunctionPass());
    passes.push_back(new CleanupPass());
    passes.push_back(new IdentifierRenamer());
  } else if (!ObfuscationList.empty()) {
    // Then we should schedule some passes required by the things below
    passes.push_back(createDemoteRegisterToMemoryPass());

    for (auto option : ObfuscationList) {
      switch (option) {
      case copyPass:
        passes.push_back(new Copy());
        break;
      case inlineFunctionPass:
        passes.push_back(new InlineFunctionPass());
        break;
      case bogusCFPass:
        passes.push_back(new BogusCF());
        break;
      case loopBCFPass:
        passes.push_back(createLoopSimplifyPass());
        passes.push_back(new LoopBogusCF());
        break;
      case opaquePredicatePass:
        passes.push_back(new OpaquePredicate());
        break;
      case replaceInstructionPass:
        passes.push_back(new ReplaceInstruction());
        break;
      case flattenPass:
        passes.push_back(new Flatten());
        break;
      case cleanupPass:
        passes.push_back(new CleanupPass());
        break;
      case identifierRenamerPass:
        passes.push_back(new IdentifierRenamer());
        break;
      default:
        llvm_unreachable("Unknown option set");
      }

      // Clean up pass
      passes.push_back(createPromoteMemoryToRegisterPass()); // Fix PHI
                                                             // demotions
      passes.push_back(createCFGSimplificationPass());       // Further cleanups
    }
  } else {

    // Default Pass Set
    // Demote PHIs to memory for ease of analysis
    passes.push_back(createDemoteRegisterToMemoryPass());

    // First batch of passes are trivial passes and should be run first to
    // "maximise confusion" that the later passes will introduce
    passes.push_back(new Copy());
    passes.push_back(new InlineFunctionPass());

    // Second batch of passes deal with introducing new control flow paths
    // These passes will insert stub 1.00 == 1.00 branches
    // which will be cleaned up by the OpaquePredicate pass
    // OpaquePredicate pass MUST be run before Flatten Pass because Flatten
    // will REMOVE the original branch instructions
    passes.push_back(new BogusCF());
    passes.push_back(createLoopSimplifyPass());
    passes.push_back(new LoopBogusCF());
    passes.push_back(new OpaquePredicate());

    // The next pass will obfuscated unreachable blocks by introducing junk
    passes.push_back(new ReplaceInstruction());

    // Flatten the control flow
    passes.push_back(new Flatten());

    // Clean ups
    passes.push_back(new CleanupPass()); // Remove stray metadata left over from
                                         // passes
    passes.push_back(createPromoteMemoryToRegisterPass()); // Fix PHI demotions
    passes.push_back(createCFGSimplificationPass());       // Further cleanups

    passes.push_back(new IdentifierRenamer());
    // passes.push_back(createStripDebugDeclarePass());
    // passes.push_back(createStripDeadDebugInfoPass());
  }

  return passes;
}
}

  // Schedue the passes
  // http://homes.cs.washington.edu/~bholt/posts/llvm-quick-tricks.html
static RegisterStandardPasses Y(PassManagerBuilder::EP_OptimizerLast,
                                [](const PassManagerBuilder &,
                                   PassManagerBase &PM) {

  if (noObfSchedule) {
    return;
  }

  std::vector<Pass *> passes = getPasses();

  for (Pass *pass : passes) {
    PM.add(pass);
  }
});
