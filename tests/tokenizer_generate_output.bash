#!/usr/bin/env bash
#
# Generate a test case output for the tokenizer.
# Usage: tokenizer_generate_output.bash [print_tokens executable]

set -u
shopt -s extglob

PRINT_TOKENS_EXECUTABLE=$1

if [ ! -f "${PRINT_TOKENS_EXECUTABLE}" ]; then
    echo "the 'print_tokens' executable does not exist in ${PRINT_TOKENS_EXECUTABLE}"
    exit 1
fi

for input_file in ${PWD}/tokenizer/!(*.expected).txt; do
    output_file=${input_file%.*}.expected.txt

    "${PRINT_TOKENS_EXECUTABLE}" < "${input_file}" > "${output_file}" 2>&1
    echo "wrote output to ${output_file}"
done

