#!/usr/bin/env bash
#
# Generate a test case output for the parser.
# Usage: parser_generate_output.bash [print_ast executable] [input file name]

set -u

PRINT_AST_EXECUTABLE=$1
INPUT_FILE=$2

if [ ! -f "${PRINT_AST_EXECUTABLE}" ]; then
    echo "the 'print_ast' executable does not exist in ${PRINT_AST_EXECUTABLE}"
    exit 1
fi

if [ ! -f "${INPUT_FILE}" ]; then
    echo "'${INPUT_FILE}' does not exist"
    exit 1
fi

output_file=${INPUT_FILE%.*}.expected.txt

"${PRINT_AST_EXECUTABLE}" < "${INPUT_FILE}" > "${output_file}" 2>&1
echo "wrote output to ${output_file}"
