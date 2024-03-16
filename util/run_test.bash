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

cmd="'$2' --require-existing-wisdom --dont-write-wisdom --wisdom-dir wisdom --log-level warn --emit-format json $TMP_DIR/program.jsons $TMP_DIR/input.jsons"

eval $cmd > $TMP_DIR/actual_output.jsons & pid=$!
( sleep 5 ; kill $pid ) & watcher=$!
wait $pid 2>/dev/null
ec=$?

kill $watcher 2>/dev/null
wait $watcher 2>/dev/null

if [ $ec -eq 143 ]; then
  echo -e "\033[0;31mCOMMAND TIMED OUT\033[0m: $1"
  echo "Keeping temporary files, command was:"
  echo "$cmd"
  exit 1
elif [ $ec -ne 0 ]; then
  echo -e "\033[0;31mCOMMAND FAILED\033[0m: $1"
  echo "Keeping temporary files, command was:"
  echo "$cmd"
  exit 1
fi

node util/jsonsDiff.js $TMP_DIR/actual_output.jsons $TMP_DIR/expected_output.jsons
ec=$?

if [ $ec -ne 0 ]; then
  echo -e "\033[0;31mDIFF FAILED\033[0m: $1"
  echo "Keeping temporary files, command was:"
  echo "$cmd"
  exit 1
fi

echo -e "\033[0;32mPASSED\033[0m: $1"
rm -rf $TMP_DIR