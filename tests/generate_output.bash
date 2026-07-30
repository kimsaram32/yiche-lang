#!/usr/bin/env bash
#
# Generate a test case output.
# Usage: generate_output.bash [CMake build directory] [type] [test case name]

set -u

CMAKE_BUILD_DIR=$1
TYPE=$2
TEST_CASE_NAME=$3

BIN_PRINT_TOKENS=${CMAKE_BUILD_DIR}/bin/print_tokens
BIN_PRINT_AST=${CMAKE_BUILD_DIR}/bin/print_ast

case ${TYPE} in
    tokenizer)
        BIN_PRINT=${BIN_PRINT_TOKENS}
        ;;
    parser)
        BIN_PRINT=${BIN_PRINT_AST}
        ;;
    *)
        echo "unknown test case type ${TYPE}"
        exit 1
        ;;
esac

file_input=${TYPE}/${TEST_CASE_NAME}.txt
file_output=${TYPE}/${TEST_CASE_NAME}.expected.txt

if [ ! -f ${file_input} ]; then
    echo "'${file_input}' does not exist"
    exit 1
fi

${BIN_PRINT_TOKENS} < ${file_input} > ${file_output} 2>&1
echo "wrote output to ${file_output}"
