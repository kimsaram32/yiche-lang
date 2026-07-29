#!/usr/bin/env bash

set -u
shopt -s extglob
shopt -s nullglob

BIN_PRINT_TOKENS=../build/bin/print_tokens
BIN_PRINT_AST=../build/bin/print_ast

get_output_file_path () {
    local file_input=$1

    echo ${file_input%.*}.expected.txt
}

total=0
failed=0

run_tests () {
    local type=$1
    local bin_print=$2

    for file_input in ./${type}/*([a-z]|_).txt; do
        ((total++))

        local file_output=$(get_output_file_path ${file_input})

        ${bin_print} < "${file_input}" 2>&1 | diff -u --color=auto "${file_output}" -

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

run_tests tokenizer $BIN_PRINT_TOKENS
run_tests parser $BIN_PRINT_AST

succeeded=$(( total - failed ))

printf "%s/%s tests succeeded" ${succeeded} ${total}

if [ ${failed} -ne 0 ]; then
    echo ", ${failed} failed"
    exit 1
fi
