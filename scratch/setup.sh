#!/bin/bash
set -eu

DEBUG=false
BUILD_DIR="build"
CONFIGURE_FLAGS="--enable-cxx11"

LLVM_PATH=${LLVM_PATH:-"../../../../"}

if [[ "$DEBUG" == "false" ]]; then
    echo "Building release";
    CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-optimized";
fi

echo "Configure flags: $CONFIGURE_FLAGS"
mkdir -p $BUILD_DIR
echo "================= Configuring LLVM ==============="
(cd $BUILD_DIR && "$LLVM_PATH/configure" $CONFIGURE_FLAGS)
echo "================= Configuring Project ==============="
(cd "$BUILD_DIR/projects" && mkdir -p LLVM-Obfuscator \
    && cd LLVM-Obfuscator && ../../../../configure $CONFIGURE_FLAGS)
echo "================= Building LLVM ==============="
(cd $BUILD_DIR && make -j 4)
echo "================= Building Project ==============="
(cd "$BUILD_DIR/projects/LLVM-Obfuscator" && make -j 4)
mkdir -p test
make
echo "Built and setup"
