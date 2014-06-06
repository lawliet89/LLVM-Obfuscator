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

OUTPUT=resilience.txt
PROGRAMS=(mergesort hanoi quicksort bubblesort)
BUILD_DIR=build
OBF_BUILD="$BUILD_DIR/projects/LLVM-Obfuscator/Release+Asserts"

CLANG="$BUILD_DIR/Release+Asserts/bin/clang++ -Wall -std=c++11"
OPT="$BUILD_DIR/Release+Asserts/bin/opt"
OPT_FLAG="-load ${OBF_BUILD}/lib/LLVMObfuscatorTransforms.so"
OBF_BASE="build/projects/LLVM-Obfuscator"

BCF_FLAG="-bogusCFPass -opaquePredicatePass\
    -replaceInstructionPass"
LOOP_FLAG="-loopBCFPass -opaquePredicatePass\
    -replaceInstructionPass"
FLATTEN_FLAGS="-flattenPass -opaquePredicatePass\
    -replaceInstructionPass"
INLINE_FLAGS="-inlineFunctionPass"
COPY_FLAGS="-copyPass"

FLAGS=(\
    "-schedule-stub"\
    "-trivialObfuscation"\
    "-flattenProbability=1.0 -copyProbability=1.0 -bcfProbability=1.0"\
    "$BCF_FLAG"\
    "$BCF_FLAG -bcfProbability=0.5"\
    "$BCF_FLAG -bcfProbability=1.0"\
    "$LOOP_FLAG"\
    "$FLATTEN_FLAGS"\
    "$FLATTEN_FLAGS -flattenProbability=0.2"\
    "$FLATTEN_FLAGS -flattenProbability=1.0"\
    "$INLINE_FLAGS"\
    "$INLINE_FLAGS -inlineProbability=0.5"\
    "$COPY_FLAGS"\
    "$COPY_FLAGS -copyProbability=1.0"\
    "-identifierRenamerPass"\
    )

main() {
    if [[ -n "${1+1}" ]]; then
        OUTPUT=$1
    fi

    POTENCY_FLAG="-schedule-metrics -metrics-output=$OUTPUT -metrics-format=,%lu,%lu,%lu"
    POTENCY_FLAG2="-metrics -metrics-output=$OUTPUT -metrics-format=,%lu,%lu,%lu"
    (cd $OBF_BASE && make > /dev/null)
    echo "Writing results to $OUTPUT"
    echo -n "" > $OUTPUT

    # Build LLVM IR
    echo "Building LLVM IR"
    for program in ${PROGRAMS[@]}; do
        echo -e "\t$program..."
        echo -n "$program" >> $OUTPUT

        $CLANG -emit-llvm -S -o test/$program.ll $program.cpp
        # $OPT ${OPT_FLAG} -noObfSchedule -S test/$program.ll > test/$program-opt.ll

        echo "" >> $OUTPUT
    done

    for ((i = 0; i < ${#FLAGS[@]}; i++)); do
        flags="${FLAGS[$i]}"

        echo "$flags" >> $OUTPUT
        echo $flags

       for program in ${PROGRAMS[@]}; do
            echo -e "\t$program..."
            echo -n "$program" >> $OUTPUT
            $OPT -O3 ${OPT_FLAG} ${POTENCY_FLAG} test/$program.ll ${flags} -o test/${program}-obf.ll -S
            $OPT ${OPT_FLAG} -noObfSchedule -O3 ${POTENCY_FLAG2} test/$program-obf.ll ${flags} -o test/${program}-obf-O3.ll -S
            echo "" >> $OUTPUT
        done
    done
}

main "$@"
