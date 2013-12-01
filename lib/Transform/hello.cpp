#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
  struct Hello : public FunctionPass {
    static char ID;
    Hello() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
    	int count = 0;
     	errs().write_escaped(F.getName()) << '\n';

		for (Function::iterator B = F.begin(), Bend = F.end(); B !=Bend; ++B) {
			for (BasicBlock::iterator I = B -> begin(), Iend = B -> end(); I != Iend; ++I) {
				// check for instance of CallInst
				if (CallInst *call = dyn_cast<CallInst>(I)) {
					Function *calledFunction = call -> getCalledFunction();
					if (calledFunction -> getName() == "printf") ++count;
				}
			}
		}

		errs() << "\t printf called " << count << " times\n";
		return false;
    }
  };
}

char Hello::ID = 0;
static RegisterPass<Hello> X("hello", "Hello World Pass - counts the number of printf(s)", false, false);
