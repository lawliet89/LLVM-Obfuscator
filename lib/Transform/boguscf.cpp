//=== boguscf.cpp - Bogus Control Flow Obfuscation Pass -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Bogus Control Flow adds by probabilistically splitting each basic block into
// two blocks and then applying an opaque predicate to see if the block ever
// gets branched to or not.
//
// Each basic block is probabilistically split using a Bernoulli distribution
// subject to a maximum number of basic blocks being transformed per function
//
// TODO: Opaque predicate generation
// Command line options
// - bcfFunc - List of functions to apply transformation to. Default is all
// - bfcProbability - Probability that basic block is transformed. Default 0.5
// - bcfSeed - Seed for random number generator. Defaults to system time
//
// Debug types:
// - boguscf - Bogus CF related
// - cfg - View CFG of functions before and after transformation
// TODO:
//  - Mangle basic block that is not in the predicate path
//  - Indeterminate opaque predicate

#define DEBUG_TYPE "boguscf"
#include "Transform/opaque_predicate.h"
#include "Transform/obf_utilities.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/ADT/Statistic.h"
// The next include file is moved > version 3.4
#include "llvm/Analysis/Dominators.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/User.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include <algorithm>
#include <vector>
#include <chrono>
#include <random>

using namespace llvm;

static cl::list<std::string>
bcfFunc("bcfFunc", cl::CommaSeparated,
        cl::desc("Insert Bogus Control Flow only for some functions: "
                 "bcfFunc=\"func1,func2\""));

static cl::opt<double>
bcfProbability("bcfProbability", cl::init(0.2),
               cl::desc("Probability that a basic block will be split"));

static cl::opt<std::string>
bcfSeed("bcfSeed", cl::init(""),
        cl::desc("Seed for random number generator. Defaults to system time"));

static cl::opt<unsigned> bcfGlobal(
    "bcfGlobal", cl::init(4),
    cl::desc("Number of global variables in a module for opaque predicates"));

STATISTIC(NumBlocksSeen, "Number of basic blocks processed (excluding skips "
                         "due to PHI/terminator only blocks)");
STATISTIC(NumBlocksSkipped,
          "Number of blocks skipped due to PHI/terminator only blocks");
STATISTIC(NumBlocksTransformed, "Number of basic blocks transformed");

namespace {
struct BogusCF : public FunctionPass {
  static char ID;
  std::minstd_rand engine;
  std::bernoulli_distribution trial;
  StringRef metaKindName;

  BogusCF() : FunctionPass(ID), metaKindName("opaqueStub") {}

  // Initialise and check options
  virtual bool doInitialization(Module &M) {
    if (bcfProbability < 0.f || bcfProbability > 1.f) {
      LLVMContext &ctx = getGlobalContext();
      ctx.emitError("BogusCF: Probability must be between 0 and 1");
    }

    // Seed engine and create distribution
    if (!bcfSeed.empty()) {
      std::seed_seq seed(bcfSeed.begin(), bcfSeed.end());
      engine.seed(seed);
    } else {
      unsigned seed =
          std::chrono::system_clock::now().time_since_epoch().count();
      engine.seed(seed);
    }

    trial.param(
        std::bernoulli_distribution::param_type((double)bcfProbability));

    return false;
  }

  virtual bool runOnFunction(Function &F) {
    bool hasBeenModified = false;
    // If the function is declared elsewhere in other translation unit
    // we should not modify it here
    if (F.isDeclaration()) {
      return false;
    }
    if (bcfProbability == 0.f) {
      return false;
    }

    DEBUG(errs() << "bcf: Function '" << F.getName() << "'\n");

    if (ObfUtils::checkFunctionTagged(F)) {
      DEBUG(errs() << "\tFunction already obfuscated -- skipping\n");
      return false;
    }

    auto funcListStart = bcfFunc.begin(), funcListEnd = bcfFunc.end();
    if (bcfFunc.size() != 0 &&
        std::find(funcListStart, funcListEnd, F.getName()) == funcListEnd) {
      DEBUG(errs() << "\tFunction not requested -- skipping\n");
      return false;
    }

    // Use a vector to store the list of blocks for probabilistic
    // splitting into two bogus control flow for a later time
    std::vector<BasicBlock *> blocks;
    blocks.reserve(F.size());
    std::vector<PHINode *> phis;

    DEBUG(errs() << "\t" << F.size() << " basic blocks found\n");
    Twine blockPrefix = "block_";
    unsigned i = 0;
    DEBUG(errs() << "\tListing and filtering blocks\n");
    BasicBlock &entryBlock = F.getEntryBlock();
    // Get original list of blocks
    for (auto &block : F) {
      DEBUG(if (!block.hasName()) {
                  block.setName(blockPrefix + Twine(i++));
                  hasBeenModified |= true;
                });

      DEBUG(errs() << "\tBlock " << block.getName() << "\n");
      for (auto &inst : block) {
        if (PHINode *phi = dyn_cast<PHINode>(&inst)) {
          phis.push_back(phi);
        }
      }
      BasicBlock::iterator inst1 = block.begin();
      if (block.getFirstNonPHIOrDbgOrLifetime()) {
        inst1 = block.getFirstNonPHIOrDbgOrLifetime();
      }
      // We do not want to transform a basic block that is only involved with
      // terminator instruction or is a landing pad for an exception
      // c.f.
      // https://github.com/llvm-mirror/llvm/blob/release_34/lib/Transforms/Utils/DemoteRegToStack.cpp#L130
      if (isa<TerminatorInst>(inst1)) {
        DEBUG(errs() << "\t\tSkipping: PHI and Terminator only\n");
        ++NumBlocksSkipped;
        continue;
      }
      if (block.isLandingPad()) {
        DEBUG(errs() << "\t\tSkipping: Landing pad block\n");
        ++NumBlocksSkipped;
        continue;
      }

      if (&block == &entryBlock) {
        DEBUG(errs() << "\t\tSkipping: Entry block\n");
        ++NumBlocksSkipped;
        continue;
      }

      // We skip functions with InvokeInst because PHI demotions are not
      // supported with invoke edges by LLVM yet.
      TerminatorInst *terminator = block.getTerminator();
      if (isa<InvokeInst>(terminator)) {
        DEBUG(errs() << "\tFunction has InvokeInst -- skipping\n");
        return hasBeenModified;
      }

      DEBUG(errs() << "\t\tAdding block\n");
      blocks.push_back(&block);
    }

    NumBlocksSeen += blocks.size();
    DEBUG(errs() << "\t" << blocks.size() << " basic blocks remaining\n");
    if (blocks.empty()) {
      return false;
    }

    DEBUG(errs() << "\tDemoting PHI instructions to allocas\n");
    for (auto phi : phis) {
      DemotePHIToStack(phi);
    }

    LLVMContext &context = F.getContext();
    MDNode *metaNode =
        MDNode::get(context, MDString::get(context, metaKindName));
    unsigned metaKind = context.getMDKindID(metaKindName);

    DEBUG_WITH_TYPE("cfg", F.viewCFG());

    trial.reset(); // Independent per function
    DEBUG(errs() << "\tRandomly shuffling list of basic blocks\n");
    std::random_shuffle(blocks.begin(), blocks.end());

    for (BasicBlock *block : blocks) {
      DEBUG(errs() << "\tBlock " << block->getName() << "\n");

      BasicBlock::iterator inst1 = block->begin();
      if (block->getFirstNonPHIOrDbgOrLifetime()) {
        inst1 = block->getFirstNonPHIOrDbgOrLifetime();
      }

      // Now let's decide if we want to transform this block or not
      if (!trial(engine)) {
        DEBUG(errs() << "\t\tSkipping: Bernoulli trial failed\n");
        continue;
      }

      ++NumBlocksTransformed;
      auto terminator = block->getTerminator();
      bool hasSuccessors = terminator->getNumSuccessors() > 0;

      BasicBlock *successor = nullptr;
      // If this block has > 1 successors, we need to create a "successor"
      // block to be the joiner block that contains the necessary PHI nodes
      // Will be handled as per normal later on
      if (hasSuccessors && terminator->getNumSuccessors() > 1) {
        DEBUG(errs() << "\t\t>1 successor: Creating successor block\n");
        successor = block->splitBasicBlock(terminator);
        DEBUG(successor->setName(block->getName() + "_successor"));

      } else if (hasSuccessors) {
        successor = *succ_begin(block);
      }

      DEBUG(errs() << "\t\tSplitting Basic Block\n");
      BasicBlock *originalBlock = block->splitBasicBlock(inst1);
      DEBUG(originalBlock->setName(block->getName() + "_original"));
      DEBUG(errs() << "\t\tCloning Basic Block\n");
      Twine prefix = "Cloned";
      ValueToValueMapTy VMap;
      BasicBlock *copyBlock = CloneBasicBlock(originalBlock, VMap, prefix, &F);
      DEBUG(copyBlock->setName(block->getName() + "_cloned"));

      // Remap operands, phi nodes, and metadata
      DEBUG(errs() << "\t\tRemapping information\n");

      for (auto &inst : *copyBlock) {
        RemapInstruction(&inst, VMap, RF_IgnoreMissingEntries);
      }

      // If this block has a successor, we need to worry about use of Values
      // generated by this block
      if (successor) {
        DEBUG(errs() << "\t\tHandling successor use\n");
        for (auto &inst : *originalBlock) {
          DEBUG(errs() << "\t\t\t" << inst << "\n");
          DEBUG(errs() << "\t\t\t\t" << inst.getNumUses() << " Users\n");
          if (!inst.getNumUses())
            continue;
          std::vector<User *> users;
          std::vector<PHINode *> phiUsers;
          bool used = false, usedNonPhi = false;
          // The instruction object itself is the Value for the result
          for (auto user = inst.use_begin(), useEnd = inst.use_end();
               user != useEnd; ++user) {
            // User is an instruction
            Instruction *userInst = dyn_cast<Instruction>(*user);
            assert(userInst && "User is not an instruction!");
            BasicBlock *userBlock = userInst->getParent();
            // Instruction belongs to another block that is not us
            if (userBlock != copyBlock && userBlock != originalBlock) {
              used = true;
              DEBUG(errs() << "\t\t\t\tUsed in " << userBlock->getName() << ": "
                           << *userInst << "\n");
              // Find a PHI Node
              PHINode *phiCheck = dyn_cast<PHINode>(userInst);
              if (phiCheck) {
                if (phiCheck->getParent() == successor) {
                  DEBUG(errs() << "\t\t\t\t\tSuccessor PHI Node\n");
                  phiUsers.push_back(phiCheck);
                } else {
                  DEBUG(errs() << "\t\t\t\t\tNon-Successor PHI Node\n");
                  usedNonPhi = true;
                  users.push_back(*user);
                }
              } else {
                DEBUG(errs() << "\t\t\t\t\tNone-PHI Node\n");
                usedNonPhi = true;
                users.push_back(*user);
              }
#if 0
              // Check if inst is a phinode in successor
              // If it's not in a successor, it's the same as the else case
              PHINode *phiCheck = dyn_cast<PHINode>(userInst);
              if (phiCheck && phiCheck->getParent() == successor) {
                DEBUG(errs() << "\t\t\t\t\tPHI Node\n");
                if (phiCheck->getBasicBlockIndex(originalBlock) != -1) {
                  phiCheck->addIncoming(VMap[&inst], copyBlock);
                }
                // break;
              } else {
                // This is an artificially created PHINode whose value will
                // always come from the "upper" blocks as originally
                // Might only happen in a loop
                // cf
                // http://llvm.org/docs/doxygen/html/classllvm_1_1LoopInfo.html
                DEBUG(errs() << "\t\t\t\t\tNone-PHI Node\n");
                if (!phi) {
                  DEBUG(errs() << "\t\t\t\t\t\tCreating PHI Node\n");
                  // If still not, then we will create in successor
                  phi = PHINode::Create(
                      inst.getType(), 2, "",
                      successor->getFirstNonPHIOrDbgOrLifetime());
                  phi->addIncoming(&inst, originalBlock);

                }
                users.push_back(*user);
              }
#endif
            }
          }
          if (!used) {
            DEBUG(errs() << "\t\t\t\t\tNo use outside of basic block\n");
            continue;
          }

          if (usedNonPhi) {
            DEBUG(errs() << "\t\t\t\tCreating PHI Node\n");
            // If still not, then we will create in successor
            PHINode *phi =
                PHINode::Create(inst.getType(), 2, "",
                                successor->getFirstNonPHIOrDbgOrLifetime());
            phiUsers.push_back(phi);
          }
          DEBUG(errs() << "\t\t\t\tUpdating PHI Nodes\n");
          for (auto phi : phiUsers) {
            DEBUG(errs() << "\t\t\t\t" << *phi << "\n");
            if (phi->getBasicBlockIndex(originalBlock) == -1)
              phi->addIncoming(&inst, originalBlock);
            if (phi->getBasicBlockIndex(copyBlock) == -1)
              phi->addIncoming(VMap[&inst], copyBlock);
            DEBUG(errs() << "\t\t\t\t\tUpdating use\n");
            // Update users
            for (User *user : users) {
              user->replaceUsesOfWith(&inst, phi);
            }
#if 0
            DEBUG(errs()
                  << "\t\t\t\t\tAdding missing incomings for predecessors\n");
            // Add incoming phi nodes for all the successor's predecessors
            // to point to itself
            // This is because the Value was used not in a PHINode but from
            // our own created one, then the Value must have only been
            // produced
            // in the block that we just split. And thus not going to be
            // changed

            for (auto pred = pred_begin(successor),
                      predEnd = pred_end(successor);
                 pred != predEnd; ++pred) {
              DEBUG(errs() << "\t\t\t\t\t\t" << (*pred)->getName() << "\n");
              if (*pred != originalBlock && *pred != copyBlock &&
                  phi->getBasicBlockIndex(*pred) == -1) {
                DEBUG(errs() << "\t\t\t\t\t\t\tAdding\n");
                phi->addIncoming(phi, *pred);
              }
            }
#endif

            DEBUG(errs() << "\t\t\t\t\tDemoting PHI Node to stack\n");
            DemotePHIToStack(phi);
          }
        }

#if 0
        // Now iterate through all the PHINode of successor to see if there
        // are any incoming blocks from the original block but not from the
        // clone block -- could be due to constant Value for example
        for (auto &inst : *successor) {
          if (PHINode *phi = dyn_cast<PHINode>(&inst)) {
            bool phiOriginalBlock = false, phiClonedBlock = false;
            Value *originalValue = nullptr;

            for (unsigned j = 0, jEnd = phi->getNumIncomingValues(); j != jEnd;
                 ++j) {
              if (phi->getIncomingBlock(j) == originalBlock) {
                phiOriginalBlock = true;
                originalValue = phi->getIncomingValue(j);
              } else if (phi->getIncomingBlock(j) == copyBlock) {
                phiClonedBlock = true;
              }
            }
            if (!phiClonedBlock && phiOriginalBlock && originalValue) {
              phi->addIncoming(originalValue, copyBlock);
            }
          }
        }
#endif
      }

#if 0
      // Create Opaque Predicates
      std::uniform_int_distribution<int> distribution;
      std::uniform_int_distribution<int> distributionType(0, 1);
      OpaquePredicate::PredicateType type =
          OpaquePredicate::create(block, originalBlock, copyBlock, globals, [&]{
        return distribution(engine);
      },
                                  [&]()->OpaquePredicate::PredicateType{
        return static_cast<OpaquePredicate::PredicateType>(
            distributionType(engine));
      });
      DEBUG(errs() << "\t\tOpaque Predicate Created: " << type << "\n");
#endif

      // Clear the unconditional branch from the "husk" original block
      block->getTerminator()->eraseFromParent();

      // Create Opaque Predicate - will turn them into "real ones" in
      // do finalization
      // Always true for now
      Value *lhs = ConstantFP::get(Type::getFloatTy(F.getContext()), 1.0);
      Value *rhs = ConstantFP::get(Type::getFloatTy(F.getContext()), 1.0);
      FCmpInst *condition = new FCmpInst(*block, FCmpInst::FCMP_TRUE, lhs, rhs);

      // Bogus conditional branch
      BranchInst *branch = BranchInst::Create(originalBlock, copyBlock,
                                              (Value *)condition, block);
      branch->setMetadata(metaKind, metaNode);

      hasBeenModified |= true;
    }
    DEBUG_WITH_TYPE("cfg", F.viewCFG());
    if (hasBeenModified)
      ObfUtils::tagFunction(F);
    return hasBeenModified;
  }

  // Finalisation will add the necessary opaque predicates
  virtual bool doFinalization(Module &M) {
    DEBUG(errs() << "Finalising: Creating Opaque Predicates\n");

    std::uniform_int_distribution<int> distribution;
    std::uniform_int_distribution<int> distributionType(0, 1);

    LLVMContext &context = M.getContext();
    unsigned metaKind = context.getMDKindID(metaKindName);

    std::vector<GlobalVariable *> globals =
        OpaquePredicate::prepareModule(M, bcfGlobal);
    for (auto &function : M) {
      DEBUG(errs() << "\tFunction " << function.getName() << "\n");
      bool functionHasBlock = false;
      for (auto &block : function) {
        TerminatorInst *terminator = block.getTerminator();
        if (terminator->getMetadata(metaKind)) {
          functionHasBlock = true;
          DEBUG(errs() << "\t\tBlock " << block.getName() << "\n");

          // Checks
          assert(terminator->getNumSuccessors() == 2 &&
                 "Stub terminator block should only have 2 successors");
          Instruction &condition = *(++block.rbegin());
          assert(isa<FCmpInst>(condition) &&
                 "Penultimate instruction should be FCmpInst");
          FCmpInst *fcmp = dyn_cast<FCmpInst>(&condition);
          assert(fcmp->getNumOperands() == 2 &&
                 "Penultimate instruction should have two operands");
          assert(
              fcmp->getPredicate() == FCmpInst::FCMP_TRUE &&
              "Penultimate instruction should have an always true predicate");

          BasicBlock *trueBlock = terminator->getSuccessor(0);
          BasicBlock *falseBlock = terminator->getSuccessor(1);
          terminator->eraseFromParent();
          fcmp->eraseFromParent();

          OpaquePredicate::PredicateType type =
              OpaquePredicate::create(&block, trueBlock, falseBlock, globals,
                                      [&] { return distribution(engine); },
                                      [&]()->OpaquePredicate::PredicateType {
                return static_cast<OpaquePredicate::PredicateType>(
                    distributionType(engine));
              });
          DEBUG(errs() << "\t\tOpaque Predicate Created: " << type << "\n");
        }
      }

      if (functionHasBlock) {
        DEBUG(errs() << "\tPromoting allocas to registers\n");
        // Promote alloca to registers
        std::vector<AllocaInst *> allocas;
        BasicBlock &entryBlock = function.getEntryBlock();
        for (Instruction &inst : entryBlock) {
          if (AllocaInst *alloca = dyn_cast<AllocaInst>(&inst)) {
            if (isAllocaPromotable(alloca)) {
              allocas.push_back(alloca);
            }
          }
        }

        // Build dominator tree
        DominatorTree &DT = getAnalysis<DominatorTree>();
        DT.getBase().recalculate(function);
        PromoteMemToReg(allocas, DT);

        DEBUG_WITH_TYPE("cfg", function.viewCFG());
      }
    }
    DEBUG(errs() << "BogusCF: Done.\n");
    return true;
  }
  virtual void getAnalysisUsage(AnalysisUsage &Info) const {
    Info.addRequired<DominatorTree>();
  }
};
}
char BogusCF::ID = 0;
static RegisterPass<BogusCF>
X("boguscf", "Insert bogus control flow paths into basic blocks", false, false);

// TODO: Optimise the scheduling of these passes
// http://homes.cs.washington.edu/~bholt/posts/llvm-quick-tricks.html
static RegisterStandardPasses Y(PassManagerBuilder::EP_OptimizerLast,
                                [](const PassManagerBuilder &,
                                   PassManagerBase &PM) {
  PM.add(new BogusCF());
});
