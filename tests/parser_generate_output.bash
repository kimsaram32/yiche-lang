#!/usr/bin/env bash
#
# Generate a test case output for the parser.
# Usage: parser_generate_output.bash [print_ast executable]

set -u

PRINT_AST_EXECUTABLE=$1

if [ ! -f "${PRINT_AST_EXECUTABLE}" ]; then
    echo "the 'print_ast' executable does not exist in ${PRINT_AST_EXECUTABLE}"
    exit 1
fi

for input_file in ${PWD}/parser/*.yiche; do
    output_file=${input_file%.*}.expected.txt

    "${PRINT_AST_EXECUTABLE}" < "${input_file}" > "${output_file}" 2>&1
    echo "wrote output to ${output_file}"
done
