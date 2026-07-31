#!/usr/bin/env bash
#
# Run all test cases.
# Usage: run_tests.bash [print_tokens executable] [print_ast executable] [run_evaluator executable]

set -u
shopt -s extglob
shopt -s nullglob

PRINT_TOKENS_EXECUTABLE=$1
PRINT_AST_EXECUTABLE=$2
RUN_EVALUATOR_EXECUTABLE=$3

get_output_file_path () {
    local file_input=$1

    echo ${file_input%.*}.expected.txt
}

total=0
failed=0

run_tests () {
    local type=$1
    local executable=$2
    local input_ext=$3

    for file_input in ./${type}/*([a-z]|_).${input_ext}; do
        ((total++))

        local file_output=$(get_output_file_path ${file_input})

        ${executable} < "${file_input}" 2>&1 | diff -u --color=auto "${file_output}" -

        local status=$?

        printf "%s: %s" ${type} ${file_input##*/}

        if [ ${status} -eq 0 ]; then
            echo " succeeded"
        else
            ((failed++))
            echo " failed"
        fi
    done
}

run_tests tokenizer ${PRINT_TOKENS_EXECUTABLE} txt
run_tests parser ${PRINT_AST_EXECUTABLE} yiche
run_tests evaluator ${RUN_EVALUATOR_EXECUTABLE} yiche

succeeded=$(( total - failed ))

printf "%s/%s tests succeeded" ${succeeded} ${total}

if [ ${failed} -ne 0 ]; then
    echo ", ${failed} failed"
    exit 1
fi
