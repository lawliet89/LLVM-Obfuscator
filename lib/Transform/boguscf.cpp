#include <algorithm>
#include <vector>
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

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
    // TODO: Paramaterise these numbers
    int max_bogus_blocks = 10;

    // If the function is declared elsewhere in other translation unit
    // we should not modify it here
    if (F.isDeclaration()) {
      return false;
    }

    DEBUG(F.viewCFG());

    // Use a vector to store the list of blocks for probabilistic
    // splitting into two bogus control flow for a later time
    std::vector<BasicBlock *> blocks;
    blocks.reserve(F.size());

    for (Function::iterator B = F.begin(), BEnd = F.end(); B != BEnd; ++B) {
      blocks.push_back((BasicBlock *)B);
    }

    std::random_shuffle(blocks.begin(), blocks.end());

    int bogus_blocks = 0;
    while (bogus_blocks < max_bogus_blocks && !blocks.empty()) {
      break;
    }

    return hasBeenModified;
  }
};
}

char BogusCF::ID = 0;
static RegisterPass<BogusCF> X("boguscf", "Insert bogus control flow paths \
into basic blocks",
                               false, false);
