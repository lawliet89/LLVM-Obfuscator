#include <algorithm>
#include <vector>
#include "llvm/Pass.h"
#include "llvm/IR/Value.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/User.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"

#define DEBUG_TYPE "boguscf"

using namespace llvm;

/*
        TODO:
                - Probabilistic insertion
                - More than two bogus flow based
                - External opaque predicate or generation or more extensive
   generation
*/
namespace {
struct BogusCF : public FunctionPass {
  static char ID;
  BogusCF() : FunctionPass(ID) {}

  virtual bool runOnFunction(Function &F) {
    bool hasBeenModified = false;
    // If the function is declared elsewhere in other translation unit
    // we should not modify it here
    if (F.isDeclaration()) {
      return false;
    }
    DEBUG_WITH_TYPE("opt", errs() << "bcf: Function '" << F.getName() << "'\n");

    // Use a vector to store the list of blocks for probabilistic
    // splitting into two bogus control flow for a later time
    std::vector<BasicBlock *> blocks;
    blocks.reserve(F.size());

    DEBUG_WITH_TYPE("opt", errs() << "\t" << F.size()
                                  << " basic blocks found\n");
    // DEBUG(F.viewCFG());
    for (Function::iterator B = F.begin(), BEnd = F.end(); B != BEnd; ++B) {
      blocks.push_back((BasicBlock *)B);
    }

    // std::random_shuffle(blocks.begin(), blocks.end());
    unsigned i = 0;
    for (BasicBlock *block : blocks) {
      DEBUG_WITH_TYPE("opt", errs() << "\tBlock " << i << "\n");
      DEBUG_WITH_TYPE("opt", errs() << "\t\tSplitting Basic Block\n");
      BasicBlock::iterator inst1 = block->begin();
      if (block->getFirstNonPHIOrDbgOrLifetime()) {
        inst1 = block->getFirstNonPHIOrDbgOrLifetime();
      }

      // We do not want to split a basic block that is only involved with some
      // terminator instruction
      if (isa<TerminatorInst>(inst1))
        continue;

      auto terminator = block->getTerminator();

      if (!isa<ReturnInst>(terminator) && terminator->getNumSuccessors() > 1) {
        DEBUG_WITH_TYPE("opt", errs() << "\t\tSkipping: >1 successor\n");
        continue;
      }

      // 1 Successor or return block
      BasicBlock *successor;
      if (!isa<ReturnInst>(terminator)) {
        successor = *succ_begin(block);
      }

      BasicBlock *originalBlock = block->splitBasicBlock(inst1);
      DEBUG_WITH_TYPE("opt", errs() << "\t\tCloning Basic Block\n");
      Twine prefix = "cloned";
      ValueToValueMapTy VMap;
      BasicBlock *copyBlock = CloneBasicBlock(originalBlock, VMap, prefix, &F);

      // Remap operands, phi nodes, and metadata
      DEBUG_WITH_TYPE("opt", errs() << "\t\tRemapping information\n");

      for (auto &inst : *copyBlock) {
        RemapInstruction(&inst, VMap, RF_IgnoreMissingEntries);
      }

      for (auto &inst : *originalBlock) {
        // The instruction object itself is the Value for the result
        errs() << "\t\t\t" << inst << ": ";
        bool usedLater = false;
        for (auto use = inst.use_begin(), useEnd = inst.use_end();
             use != useEnd; ++use) {
          if (Instruction *userInst = dyn_cast<Instruction>(*use)) {
            BasicBlock *userBlock = userInst->getParent();
            if (userBlock != copyBlock && userBlock != originalBlock){
              usedLater = true;
              errs() << "Used Later";
            }
            if (PHINode *phi = dyn_cast<PHINode>(userInst)) {
              errs() << " PHINode";
              // Update PHINode
              // TODO: Handle if it wasn't
              phi->addIncoming(VMap[&inst], copyBlock);
            }

          }
        }
        if (!usedLater) {
          errs() << "NOT used later";
        }
        errs() << "\n";
      }

      // Clear the unconditional branch from the "husk" original block
      block->getTerminator()->eraseFromParent();

      // Create Opaque Predicate
      // Always true for now
      Value *lhs = ConstantFP::get(Type::getFloatTy(F.getContext()), 1.0);
      Value *rhs = ConstantFP::get(Type::getFloatTy(F.getContext()), 1.0);
      FCmpInst *condition = new FCmpInst(*block, FCmpInst::FCMP_TRUE, lhs, rhs);

      // Bogus conditional branch
      BranchInst::Create(originalBlock, copyBlock, (Value *)condition, block);

      // Handle phi nodes in successor block

      hasBeenModified |= true;
      ++i;
    }
    DEBUG_WITH_TYPE("cfg", F.viewCFG());
    return hasBeenModified;
  }

  virtual void getAnalysisUsage(AnalysisUsage &Info) const {
    Info.addRequired<DominatorTree>();
  }
};
}

char BogusCF::ID = 0;
static RegisterPass<BogusCF>
X("boguscf", "Insert bogus control flow paths into basic blocks", false, false);
