#!/bin/bash
set -eu

OUTPUT=results.txt
SIZES=(10 50 100 200 500 1000 2500 5000 10000 100000 1000000 10000000 100000000)
SORTS=(mergesort quicksort stack-sort)

main() {
    if [[ -n "${1+1}" ]]; then
        OUTPUT=$1
    fi

    echo "Building..."
    make -j 4

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


}

main "$@"
