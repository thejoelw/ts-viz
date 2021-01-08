#!/bin/sh

sort --unique ts-viz.files | xargs ls -1d 2>/dev/null > ts-viz.files.tmp
mv ts-viz.files.tmp ts-viz.files

printf "src/\nthird_party/\nthird_party/spdlog/include/\nthird_party/rapidjson/include/\nbuild-qtc/src/\n" > ts-viz.includes
