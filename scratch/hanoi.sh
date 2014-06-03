#!/bin/bash
set -eu
# Flags
# 1 - Default
# 2 - Trivial
# 3 - Maximum
# 4 - BCF Default
# 5 - BCF 0.5
# 6 - BCF 1.0
# 7 - Loop BCF
# 8 - Flatten Default
# 9 - Flatten 0.2
# 10 - Flatten 1.0

OUTPUT=hanoi.txt
SIZES=(10 15 20 21 22 23 24 25 26 27 28 29 30 31 32)

BCF_FLAG="-mllvm -bogusCFPass -mllvm -opaquePredicatePass\
    -mllvm -replaceInstructionPass"
LOOP_FLAG="-mllvm -loopBCFPass -mllvm -opaquePredicatePass\
    -mllvm -replaceInstructionPass"
FLATTEN_FLAGS="-mllvm -flattenPass -mllvm -opaquePredicatePass\
    -mllvm -replaceInstructionPass"

FLAGS=(\
    "-mllvm -trivialObfuscation"\
    "-mllvm -flattenProbability=1.0 -mllvm -copyProbability=1.0 -mllvm -bcfProbability=1.0 -mllvm -inlineProbability=1.0"\
    "$BCF_FLAG"\
    "$BCF_FLAG -mllvm -bcfProbability=0.5"\
    "$BCF_FLAG -mllvm -bcfProbability=1.0"\
    "$LOOP_FLAG"\
    "$FLATTEN_FLAGS"\
    "$FLATTEN_FLAGS -mllvm -flattenProbability=0.2"\
    "$FLATTEN_FLAGS -mllvm -flattenProbability=1.0"\
    )

main() {
    if [[ -n "${1+1}" ]]; then
        OUTPUT=$1
    fi

    echo "Building..."
    export OBF_FLAGS=""
    make

    echo "Writing results to $OUTPUT"
    echo -n "" > $OUTPUT

    # tempdir=$(mktemp -d "/tmp/obfuscator.XXXXXXXXXX") ||\
    # { echo "Failed to create temp directory"; exit 1; }
    # trap "rm -rf $tempdir" EXIT

    tempdir=temp
    rm -rf $tempdir
    mkdir -p $tempdir

    for size in ${SIZES[@]}; do
        echo -ne "\t$size" >> $OUTPUT
    done
    echo "" >> $OUTPUT


    for size in ${SIZES[@]}; do
        echo -ne "\t" >> $OUTPUT
        (/usr/bin/time -f "%e" "test/hanoi" "$size" > /dev/null) 2>&1 | tr '\n' ' ' >>  $OUTPUT
    done
    echo "" >> $OUTPUT

    for size in ${SIZES[@]}; do
        echo -ne "\t" >> $OUTPUT
        (/usr/bin/time -f "%e" "test/hanoi-obf" "$size" > /dev/null) 2>&1 | tr '\n' ' ' >>  $OUTPUT
    done
    echo "" >> $OUTPUT

    for ((i = 0; i < ${#FLAGS[@]}; i++)); do
        flags="${FLAGS[$i]}"
        rm test/hanoi-obf
        (export OBF_FLAGS="$flags"; make)
        echo "$flags" >> $OUTPUT
        for size in ${SIZES[@]}; do
            echo -ne "\t" >> $OUTPUT
            (/usr/bin/time -f "%e" "test/hanoi-obf" "$size" > /dev/null) 2>&1 | tr '\n' ' ' >>  $OUTPUT
        done
        echo "" >> $OUTPUT
    done
}

main "$@"
