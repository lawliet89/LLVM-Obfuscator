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

OUTPUT=bubble.txt
SIZES=(1000 2000 3000 4000 5000 7500 10000 25000 50000 100000 200000)
SORTS=(bubblesort)

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

    echo "Generating sequences..."
    for size in ${SIZES[@]}; do
        echo -e "\t$size"
        test/generator $size > "$tempdir/input-$size.txt"
        echo -ne "\t$size" >> $OUTPUT
    done
    echo "" >> $OUTPUT

    for sort in ${SORTS[@]}; do
        echo "Handling $sort..."
        echo -n "$sort" >> $OUTPUT

        for size in ${SIZES[@]}; do
            echo -ne "\t" >> $OUTPUT
            (/usr/bin/time -f "%e" "test/$sort" "$tempdir/input-$size.txt"\
                    > "$tempdir/$sort-$size.txt") 2>&1 | tr '\n' ' ' >>  $OUTPUT
        done
        echo "" >> $OUTPUT

        echo -n "${sort}-obf" >> $OUTPUT
        for size in ${SIZES[@]}; do
            echo -ne "\t" >> $OUTPUT
            (/usr/bin/time -f "%e" "test/${sort}-obf" "$tempdir/input-$size.txt"\
                    > "$tempdir/obf-$sort-$size.txt") 2>&1 | tr '\n' ' ' >>  $OUTPUT

            diff "$tempdir/obf-$sort-$size.txt" "$tempdir/$sort-$size.txt"\
             > /dev/null || echo -ne " DIFFER" >> $OUTPUT
        done
        echo "" >> $OUTPUT
    done

    for ((i = 0; i < ${#FLAGS[@]}; i++)); do
        flags="${FLAGS[$i]}"
        make clean-obf
        (export OBF_FLAGS="$flags"; make)
        echo "$flags" >> $OUTPUT

        for sort in ${SORTS[@]}; do
            echo -n "${sort}-obf" >> $OUTPUT
            for size in ${SIZES[@]}; do
                echo -ne "\t" >> $OUTPUT
                (/usr/bin/time -f "%e" "test/${sort}-obf" "$tempdir/input-$size.txt"\
                        > "$tempdir/obf-$sort-$size.txt") 2>&1 | tr '\n' ' ' >>  $OUTPUT

                diff "$tempdir/obf-$sort-$size.txt" "$tempdir/$sort-$size.txt"\
                 > /dev/null || echo -ne " DIFFER" >> $OUTPUT
            done
            echo "" >> $OUTPUT
        done
    done
}

main "$@"
