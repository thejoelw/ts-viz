#!/bin/bash

set -e
set -x

# Change working directory to script location
pushd `dirname $0` > /dev/null

rm -rf imgui readerwriterqueue spdlog rapidjson argparse

git clone git@github.com:ocornut/imgui.git
pushd imgui
git checkout v1.75
for filename in *.cpp examples/imgui_impl_glfw.cpp examples/imgui_impl_opengl3.cpp ; do
	cat <(echo '#include "defs/ENABLE_GRAPHICS.h"') <(echo '#if ENABLE_GRAPHICS') <(echo) "$filename" <(echo) <(echo '#endif') > "../../src/imgui/$(basename $filename)"
done
popd

git clone git@github.com:cameron314/readerwriterqueue.git
pushd readerwriterqueue
git checkout v1.0.3
popd

# curl -Lo imgui/imgui_impl_glfw.h https://raw.githubusercontent.com/ocornut/imgui/v1.65/examples/imgui_impl_glfw.h
# curl -Lo imgui/imgui_impl_glfw.cpp https://raw.githubusercontent.com/ocornut/imgui/v1.65/examples/imgui_impl_glfw.cpp
# curl -Lo imgui/imgui_impl_opengl3.h https://raw.githubusercontent.com/ocornut/imgui/v1.65/examples/imgui_impl_opengl3.h
# curl -Lo imgui/imgui_impl_opengl3.cpp https://raw.githubusercontent.com/ocornut/imgui/v1.65/examples/imgui_impl_opengl3.cpp

git clone git@github.com:gabime/spdlog.git
pushd spdlog
git checkout v1.8.1
popd

git clone git@github.com:Tencent/rapidjson.git
pushd rapidjson
git checkout 0ccdbf364c577803e2a751f5aededce935314313
popd

git clone git@github.com:p-ranav/argparse.git
pushd argparse
git checkout 9903a22904fed8176c4a1f69c4b691304b23c78e
popd


popd > /dev/null
