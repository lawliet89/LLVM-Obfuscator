#include <algorithm>
#include <vector>
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
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
      BasicBlock *originalBlock = block->splitBasicBlock(inst1);

      DEBUG_WITH_TYPE("opt", errs() << "\t\tCloning Basic Block\n");
      Twine prefix = "cloned";
      ValueToValueMapTy VMap;
      BasicBlock *copyBlock = CloneBasicBlock(originalBlock, VMap, prefix, &F);

      // Remap operands, phi nodes, and metadata
      DEBUG_WITH_TYPE("opt", errs() << "\t\tRemapping information\n");
      for (auto &inst : *copyBlock) {
        // Operands
        for (auto operand = inst.op_begin(), operandEnd = inst.op_end();
             operand != operandEnd; ++operand) {
          Value *v = MapValue(*operand, VMap);
          if (v)
            *operand = v;
        }
        // Phi nodes
        if (PHINode *phi = dyn_cast<PHINode>(&inst)) {
          for (unsigned i = 0, e = phi->getNumIncomingValues(); i != e; ++i) {
            Value *v = MapValue(phi->getIncomingBlock(i), VMap);
            if (v)
              phi->setIncomingBlock(i, cast<BasicBlock>(v));
          }
        }

        // Metadata
        SmallVector<std::pair<unsigned, MDNode *>, 4> metas;
        inst.getAllMetadata(metas);
        for (auto &meta : metas) {
          MDNode *oldMeta = meta.second;
          MDNode *newMeta = MapValue(oldMeta, VMap);
          if (oldMeta != newMeta)
            inst.setMetadata(meta.first, newMeta);
        }
      }

      block->getTerminator()->eraseFromParent();

      // Create Opaque Predicate
      // Always true for now
      Value *lhs = ConstantFP::get(Type::getFloatTy(F.getContext()), 1.0);
      Value *rhs = ConstantFP::get(Type::getFloatTy(F.getContext()), 1.0);
      FCmpInst *condition = new FCmpInst(*block, FCmpInst::FCMP_TRUE, lhs, rhs);

      // Bogus conditional branch
      BranchInst::Create(originalBlock, copyBlock,(Value *) condition, block);

      hasBeenModified |= true;
      ++i;
    }

    // DEBUG(F.viewCFG());

    return hasBeenModified;
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    // AU.addRequired<DCE>();
  }
};
}

char BogusCF::ID = 0;
static RegisterPass<BogusCF> X("boguscf", "Insert bogus control flow paths \
into basic blocks",
                               false, false);
