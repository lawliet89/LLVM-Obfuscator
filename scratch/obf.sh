#!/bin/bash
set -eu
LLVM_BUILD="build/Release+Asserts"
OBF_BASE="build/projects/LLVM-Obfuscator"
OBF_BUILD="build/projects/LLVM-Obfuscator/Release+Asserts"
# (cd ${OBF_BASE} && make) >&2
${LLVM_BUILD}/bin/clang++ -Xclang -load -Xclang \
    ${OBF_BUILD}/lib/LLVMObfuscatorTransforms.so $@
