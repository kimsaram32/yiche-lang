#!/usr/bin/env bash
#
# Generate a test case output for the parser.
# Usage: generate_tokenizer_parser.bash [CMake build directory] [input file name]

set -u

CMAKE_BUILD_DIR=$1
FILE_INPUT=$2

BIN_PRINT_AST=${CMAKE_BUILD_DIR}/bin/print_ast

if [ ! -f ${BIN_PRINT_AST} ]; then
    echo "the 'print_ast' executable does not exist in ${BIN_PRINT_AST}"
    exit 1
fi

if [ ! -f ${FILE_INPUT} ]; then
    echo "'${FILE_INPUT}' does not exist"
    exit 1
fi

file_output=${FILE_INPUT%.*}.expected.txt

${BIN_PRINT_AST} < ${FILE_INPUT} > ${file_output} 2>&1
echo "wrote output to ${file_output}"
