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
#include "Transform/boguscf.h"
#include "Transform/flatten.h"
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

static cl::list<std::string> copyFunc("copyFunc", cl::CommaSeparated,
                                      cl::desc("Only copy some functions: "
                                               "copyFunc=\"func1,func2\""));

static cl::opt<double> copyProbability(
    "copyProbability", cl::init(0.5),
    cl::desc(
        "Probability that a function will be copied if list is not specified"));

static cl::opt<double>
    copyReplaceProbability("copyReplaceProbability", cl::init(0.5),
                           cl::desc("Probability that an invocation to a "
                                    "copied function will be replaced"));

static cl::opt<std::string> copySeed(
    "copySeed", cl::init(""),
    cl::desc("Seed for random number generator. Defaults to system time"));

static cl::opt<bool> copyEnsureEligibility(
    "copyEnsureEligibility", cl::init(true),
    cl::desc("Check and ensure that functions can be obfuscated by at least "
             "one obfuscating pass before copying it, and then marking it from "
             "a randomly list of eligible ones. Defaults to true"));

static cl::opt<bool> copyEnsureReplacement(
    "copyEnsureReplacement", cl::init(true),
    cl::desc("Check and ensure that at least one use is replaced with clone"));

bool Copy::runOnModule(Module &M) {
  // Initialise
  if (copyProbability < 0.f || copyProbability > 1.f) {
    LLVMContext &ctx = getGlobalContext();
    ctx.emitError("Copy: Probability must be between 0 and 1");
  }

  if (copyReplaceProbability < 0.f || copyReplaceProbability > 1.f) {
    LLVMContext &ctx = getGlobalContext();
    ctx.emitError("Copy: copyReplaceProbability must be between 0 and 1");
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
  trialReplace.param(
      std::bernoulli_distribution::param_type((double)copyReplaceProbability));

  bool hasBeenModified = false;
  auto funcListStart = copyFunc.begin(), funcListEnd = copyFunc.end();
  std::vector<Function *> cloneList;
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;

    DEBUG(errs() << "Copy: Function '" << F.getName() << "'\n");
    if (copyFunc.empty()) {
      // Play dice
      if (!trial(engine)) {
        DEBUG(errs() << "\tSkipping: Bernoulli trial failed\n");
        continue;
      }
    } else {
      if (std::find(funcListStart, funcListEnd, F.getName()) == funcListEnd) {
        DEBUG(errs() << "\tFunction not requested -- skipping\n");
        continue;
      }
    }
    DEBUG(errs() << "\tMark for Cloning\n");
    cloneList.push_back(&F);
  }

  for (Function *F : cloneList) {
    DEBUG(errs() << F->getName() << ":\n");

    ObfUtils::ObfType mustObfType = ObfUtils::NoneObf;
    if (copyEnsureEligibility) {
      std::vector<ObfUtils::ObfType> eligible;
      DEBUG(errs() << "\tChecking eligibility:\n");

      if (BogusCF::isEligible(*F)) {
        DEBUG(errs() << "\t\tBogusCF\n");
        eligible.push_back(ObfUtils::BogusCFObf);
      }
      if (Flatten::isEligible(*F)) {
        DEBUG(errs() << "\t\tFlatten\n");
        eligible.push_back(ObfUtils::FlattenObf);
      }

      if (eligible.empty()) {
        DEBUG(errs() << "\t\tNon eligible -- skipping\n");
        continue;
      }

      // Choose a random one
      std::uniform_int_distribution<unsigned> distribution(0,
                                                           eligible.size() - 1);
      mustObfType = eligible[distribution(engine)];
    }

    std::vector<Instruction *> users;
    // Get list of users
    for (auto user = F->use_begin(), useEnd = F->use_end(); user != useEnd;
         ++user) {
      Instruction *inst = dyn_cast<Instruction>(*user);
      if (!inst) {
        continue;
      }
      assert((isa<InvokeInst>(inst) || isa<CallInst>(inst)) &&
             "Function is not used by an InvokeInst or CallInst");

      users.push_back(inst);
    }

    if (copyEnsureReplacement && users.size() < 2) {
      DEBUG(errs() << "\t\tFunction has < 2 users -- skipping\n");
      continue;
    }

    hasBeenModified |= true;

    // Refer to http://git.io/2mp3-Q
    std::vector<Type *> ArgTypes;
    ValueToValueMapTy VMap;
    for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end();
         I != E; ++I)
      ArgTypes.push_back(I->getType());
    FunctionType *FTy =
        FunctionType::get(F->getFunctionType()->getReturnType(), ArgTypes,
                          F->getFunctionType()->isVarArg());

    Twine cloneName("");
    DEBUG(cloneName = cloneName.concat(F->getName()));
    Function *clone = Function::Create(FTy, F->getLinkage(), cloneName, &M);

    Function::arg_iterator DestI = clone->arg_begin();
    for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end();
         I != E; ++I) {
      DestI->setName(I->getName()); // Copy the name over...
      VMap[I] = DestI++;            // Add mapping to VMap
    }
    SmallVector<ReturnInst *, 8> Returns; // Ignore returns cloned.
    CloneFunctionInto(clone, F, VMap, true, Returns);

    // Tag cloned function
    if (mustObfType != ObfUtils::NoneObf) {
      std::string type;
      switch (mustObfType) {
      case ObfUtils::BogusCFObf:
        type = "boguscf";
        break;
      case ObfUtils::FlattenObf:
        type = "flatten";
        break;
      default:
        llvm_unreachable("Invalid obfuscation type");
      }
      ObfUtils::tagFunction(*clone, ObfUtils::CopyObf,
                            MDString::get(F->getContext(), type.c_str()));
    }

    unsigned replacementCount = 0;
    unsigned userCount = users.size();
    // if copyEnsureReplacement is not true, will only run once
    do {
      for (auto inst : users) {
        if (replacementCount > (userCount - 2)) {
          break;
        }
        // Trial
        if (trialReplace(engine)) {
          DEBUG(errs() << "\t\tReplacing use in " << *inst << "\n");

          if (CallInst *call = dyn_cast<CallInst>(inst)) {
            call->setCalledFunction((Value *)clone);
          } else if (InvokeInst *invoke = dyn_cast<InvokeInst>(inst)) {
            invoke->setCalledFunction((Value *)clone);
          } else {
            llvm_unreachable("Unknown instruction type");
          }
          ++replacementCount;
        }
      }

    } while (copyEnsureReplacement && replacementCount == 0);

  }

  return hasBeenModified;
}

void Copy::tagFunction(Function &F, ObfUtils::ObfType type) {
  ObfUtils::tagFunction(F, ObfUtils::CopyObf,
                        MDString::get(F.getContext(), obfString(type)));
}

bool Copy::isFunctionTagged(Function &F, ObfUtils::ObfType type) {
  MDNode *md = ObfUtils::checkFunctionTagged(F, type);
  if (!md)
    return false;
  if (md->getNumOperands() < 1)
    return false;
  MDString *str = dyn_cast<MDString>(md->getOperand(0));
  if (!str)
    return false;
  StringRef expectedString = obfString(type);
  if (str->getString() == expectedString)
    return true;
  return false;
}

StringRef Copy::obfString(ObfUtils::ObfType type) {
  switch (type) {
  case ObfUtils::BogusCFObf:
    return StringRef("boguscf");
    break;
  case ObfUtils::FlattenObf:
    return StringRef("flatten");
    break;
  default:
    llvm_unreachable("Invalid obfuscation type");
  }
}

char Copy::ID = 0;
static RegisterPass<Copy> X("copy", "Copy function pass", false, false);
