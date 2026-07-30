#!/usr/bin/env bash
#
# Generate a test case output for the tokenizer.
# Usage: tokenizer_generate_output.bash [print_tokens executable] [input file name]

set -u

PRINT_TOKENS_EXECUTABLE=$1
INPUT_FILE=$2

if [ ! -f "${PRINT_TOKENS_EXECUTABLE}" ]; then
    echo "the 'print_tokens' executable does not exist in ${PRINT_TOKENS_EXECUTABLE}"
    exit 1
fi

if [ ! -f "${INPUT_FILE}" ]; then
    echo "'${INPUT_FILE}' does not exist"
    exit 1
fi

output_file=${INPUT_FILE%.*}.expected.txt

"${PRINT_TOKENS_EXECUTABLE}" < "${INPUT_FILE}" > "${output_file}" 2>&1
echo "wrote output to ${output_file}"
