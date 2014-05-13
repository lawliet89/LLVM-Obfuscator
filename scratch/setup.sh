#!/bin/bash
set -eu

DEBUG=false
BUILD_DIR="build"
CONFIGURE_FLAGS=""

if [[ "$DEBUG" == "false" ]]; then
    echo "Building release";
    CONFIGURE_FLAGS="${CONFIGURE_FLAGS}--enable-optimized";
fi

mkdir -p $BUILD_DIR
(cd $BUILD_DIR && ../../../../configure "$CONFIGURE_FLAGS")
(cd "$BUILD_DIR/projects" && mkdir -p LLVM-Obfuscator \
    && cd LLVM-Obfuscator && ../../../../configure "$CONFIGURE_FLAGS")
(cd $BUILD_DIR && make -j 4)
(cd "$BUILD_DIR/projects/LLVM-Obfuscator" && make -j 4)

echo "Built and setup"
