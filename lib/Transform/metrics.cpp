//=== metrics.cpp - Potency analysis metrics ================================//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "metrics"
#include "Transform/metrics.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"

using namespace llvm;

char Metrics::ID = 0;

static cl::opt<std::string> metricsOutput(
    "metrics-output", cl::init(""),
    cl::desc("Write metrics to an output file instead of stderr"));

static cl::opt<std::string> metricsFormat(
    "metrics-format", cl::init("%lu %lu %lu"),
    cl::desc("String format for results. If none, will be verbose output"));

bool Metrics::runOnModule(Module &M) {
  unsigned long instructionCount = 0;
  unsigned long operandCount = 0;

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &inst : BB) {
        ++instructionCount;

        operandCount += inst.getNumOperands();
      }
    }
  }

  unsigned zero = 0;
  errs() << format(metricsFormat.c_str(), instructionCount, zero, zero);
  return false;
}

static RegisterPass<Metrics> X("metrics", "Potency analysis metrics pass",
                               false, true);
