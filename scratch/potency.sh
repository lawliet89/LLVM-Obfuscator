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

OUTPUT=potency.txt
PROGRAMS=(mergesort hanoi quicksort bubblesort)

BCF_FLAG="-mllvm -bogusCFPass -mllvm -opaquePredicatePass\
    -mllvm -replaceInstructionPass"
LOOP_FLAG="-mllvm -loopBCFPass -mllvm -opaquePredicatePass\
    -mllvm -replaceInstructionPass"
FLATTEN_FLAGS="-mllvm -flattenPass -mllvm -opaquePredicatePass\
    -mllvm -replaceInstructionPass"
INLINE_FLAGS="-mllvm -inlineFunctionPass"
COPY_FLAGS="-mllvm -copyPass"

FLAGS=(\
    ""\
    "-mllvm -trivialObfuscation"\
    "-mllvm -flattenProbability=1.0 -mllvm -copyProbability=1.0 -mllvm -bcfProbability=1.0"\
    "$BCF_FLAG"\
    "$BCF_FLAG -mllvm -bcfProbability=0.5"\
    "$BCF_FLAG -mllvm -bcfProbability=1.0"\
    "$LOOP_FLAG"\
    "$FLATTEN_FLAGS"\
    "$FLATTEN_FLAGS -mllvm -flattenProbability=0.2"\
    "$FLATTEN_FLAGS -mllvm -flattenProbability=1.0"\
    "$INLINE_FLAGS"\
    "$INLINE_FLAGS -mllvm -inlineProbability=0.5"\
    "$COPY_FLAGS"\
    "$COPY_FLAGS -mllvm -copyProbability=1.0"\
    "-mllvm -identifierRenamerPass"\
    )

main() {
    if [[ -n "${1+1}" ]]; then
        OUTPUT=$1
    fi

   POTENCY_FLAG="-mllvm -schedule-metrics -mllvm -metrics-output=$OUTPUT -mllvm -metrics-format=\",%lu,%lu,%lu\""

    make clean # to build library
    echo "Writing results to $OUTPUT"
    echo -n "" > $OUTPUT

    for ((i = 0; i < ${#FLAGS[@]}; i++)); do
        flags="${FLAGS[$i]}"
        make clean-obf
        make test/get_input_obf.o

        echo "$flags" >> $OUTPUT

       for program in ${PROGRAMS[@]}; do
            echo "Handling $program..."
            echo -n "$program" >> $OUTPUT
            (export OBF_FLAGS="${POTENCY_FLAG} ${flags}"; make "test/${program}-obf")
            echo "" >> $OUTPUT
        done
    done
}

main "$@"
