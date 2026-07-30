#!/usr/bin/env bash
#
# Generate a test case output for the tokenizer.
# Usage: generate_tokenizer_output.bash [CMake build directory] [input file name]

set -u

CMAKE_BUILD_DIR=$1
FILE_INPUT=$2

BIN_PRINT_TOKENS=${CMAKE_BUILD_DIR}/bin/print_tokens

if [ ! -f ${BIN_PRINT_TOKENS} ]; then
    echo "the 'print_tokens' executable does not exist in ${BIN_PRINT_TOKENS}"
    exit 1
fi

if [ ! -f ${FILE_INPUT} ]; then
    echo "'${FILE_INPUT}' does not exist"
    exit 1
fi

file_output=${FILE_INPUT%.*}.expected.txt

${BIN_PRINT_TOKENS} < ${FILE_INPUT} > ${file_output} 2>&1
echo "wrote output to ${file_output}"
