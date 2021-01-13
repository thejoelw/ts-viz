#!/bin/bash

set -e
# set -x

# echo "Variant Name: $1"
# echo "Binary: $2"

TMP_DIR=$(mktemp -d)
echo '{}' > $TMP_DIR/input.jsons
echo '[["emit","out_float",["conv",["to_ts",["cast_float",0]],["to_ts",["cast_float",0]],["cast_int64",2],false]],["emit","out_double",["conv",["to_ts",["cast_double",0]],["to_ts",["cast_double",0]],["cast_int64",2],false]]]' > $TMP_DIR/program.jsons

"$2" --log-level warning $TMP_DIR/program.jsons $TMP_DIR/input.jsons > /dev/null

echo -e "\033[0;32mFINISHED PREPARING WISDOM\033[0m: $1"

rm -rf $TMP_DIR