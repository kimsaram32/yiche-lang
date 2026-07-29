#!/usr/bin/env bash

set -u

FILE_INPUT=$1

BIN_PRINT_TOKENS=../../build/bin/print_tokens

if [ ! -f ${FILE_INPUT} ]; then
    echo "'${FILE_INPUT}' does not exist"
    exit 1
fi

file_output=${FILE_INPUT%.*}.expected.txt

${BIN_PRINT_TOKENS} < ${FILE_INPUT} > ${file_output} 2>&1
echo "wrote output to ${file_output}"
