#!/bin/bash

set -e
set -x

# echo "Binary: $1"
# echo "Input: $2"
# echo "Program: $3"
# echo "Output: $4"

TMP_DIR=$(mktemp -d)
printf "$2" > $TMP_DIR/input.jsons
printf "$3" > $TMP_DIR/program.jsons
printf "$4" | cat -n > $TMP_DIR/expected_output.jsons

"$1" --dont-write-wisdom $TMP_DIR/program.jsons $TMP_DIR/input.jsons | cat -n > $TMP_DIR/actual_output.jsons

git diff --color $TMP_DIR/expected_output.jsons $TMP_DIR/actual_output.jsons

# rm -rf $TMP_DIR
