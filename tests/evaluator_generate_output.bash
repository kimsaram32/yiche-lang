#!/usr/bin/env bash
#
# Generate a test case output for the evaluator.
# Usage: evaluator_generate_output.bash [run_evaluator executable]

set -u

RUN_EVALUATOR_EXECUTABLE=$1

if [ ! -f "${RUN_EVALUATOR_EXECUTABLE}" ]; then
    echo "the 'run_evaluator' executable does not exist in ${RUN_EVALUATOR_EXECUTABLE}"
    exit 1
fi

for input_file in ${PWD}/evaluator/*.yiche; do
  output_file=${input_file%.*}.expected.txt

  "${RUN_EVALUATOR_EXECUTABLE}" < "${input_file}" > "${output_file}" 2>&1
  echo "wrote output to ${output_file}"
done
