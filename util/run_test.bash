#!/bin/bash

# set -e
# set -x

# echo "Test Name: $1"
# echo "Binary: $2"
# echo "Input: $3"
# echo "Program: $4"
# echo "Output: $5"

TMP_DIR=$(mktemp -d)
printf "$3" > $TMP_DIR/input.jsons
printf "$4" > $TMP_DIR/program.jsons
printf "$5" > $TMP_DIR/expected_output.jsons

"$2" --dont-write-wisdom $TMP_DIR/program.jsons $TMP_DIR/input.jsons > $TMP_DIR/actual_output.jsons

if [ $? -ne 0 ]; then
  echo -e "\033[0;31mCOMMAND FAILED\033[0m: $1"
  echo "Keeping temporary files, command was:"
  echo "$2" --dont-write-wisdom $TMP_DIR/program.jsons $TMP_DIR/input.jsons
  exit 1
fi

node util/jsonsDiff.js $TMP_DIR/actual_output.jsons $TMP_DIR/expected_output.jsons

if [ $? -ne 0 ]; then
  echo -e "\033[0;31mDIFF FAILED\033[0m: $1"
  echo "Keeping temporary files, command was:"
  echo "$2" --dont-write-wisdom $TMP_DIR/program.jsons $TMP_DIR/input.jsons
  exit 1
fi

echo -e "\033[0;32mPASSED\033[0m: $1"
rm -rf $TMP_DIR