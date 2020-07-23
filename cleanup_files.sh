#!/bin/sh

sort --unique ts-viz.files | xargs ls -1d 2>/dev/null > ts-viz.files.tmp
mv ts-viz.files.tmp ts-viz.files

printf "src/\nthird_party/\nbuild-debug/src/\n" > ts-viz.includes

