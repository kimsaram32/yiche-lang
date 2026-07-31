#!/usr/bin/env bash
#
# Generate a test case output for the evaluator.
# Usage: evaluator_generate_output.bash [run_evaluator executable] [input file name]

set -u

RUN_EVALUATOR_EXECUTABLE=$1
INPUT_FILE=$2

if [ ! -f "${RUN_EVALUATOR_EXECUTABLE}" ]; then
    echo "the 'run_evaluator' executable does not exist in ${RUN_EVALUATOR_EXECUTABLE}"
    exit 1
fi

if [ ! -f "${INPUT_FILE}" ]; then
    echo "'${INPUT_FILE}' does not exist"
    exit 1
fi

output_file=${INPUT_FILE%.*}.expected.txt

"${RUN_EVALUATOR_EXECUTABLE}" < "${INPUT_FILE}" > "${output_file}" 2>&1
echo "wrote output to ${output_file}"
