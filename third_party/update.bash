#!/bin/bash

set -e
set -x

# Change working directory to script location
pushd `dirname $0` > /dev/null

rm -rf imgui readerwriterqueue spdlog rapidjson argparse ../src/imgui/*

git clone git@github.com:ocornut/imgui.git
pushd imgui
git checkout 677fe33990ac02d925da3d5a929bbbb6f01800bd
for filename in *.cpp backends/imgui_impl_glfw.cpp backends/imgui_impl_opengl3.cpp ; do
	cat <(echo '#include "defs/ENABLE_GRAPHICS.h"') <(echo '#if ENABLE_GRAPHICS') <(echo) "$filename" <(echo) <(echo '#endif') > "../../src/imgui/$(basename $filename)"
done
popd

git clone git@github.com:cameron314/readerwriterqueue.git
pushd readerwriterqueue
git checkout v1.0.5
popd

# curl -Lo imgui/imgui_impl_glfw.h https://raw.githubusercontent.com/ocornut/imgui/v1.65/examples/imgui_impl_glfw.h
# curl -Lo imgui/imgui_impl_glfw.cpp https://raw.githubusercontent.com/ocornut/imgui/v1.65/examples/imgui_impl_glfw.cpp
# curl -Lo imgui/imgui_impl_opengl3.h https://raw.githubusercontent.com/ocornut/imgui/v1.65/examples/imgui_impl_opengl3.h
# curl -Lo imgui/imgui_impl_opengl3.cpp https://raw.githubusercontent.com/ocornut/imgui/v1.65/examples/imgui_impl_opengl3.cpp

git clone git@github.com:gabime/spdlog.git
pushd spdlog
git checkout v1.9.2
popd

git clone git@github.com:Tencent/rapidjson.git
pushd rapidjson
git checkout 2e8f5d897d9d461a7273b4b812b0127f321b1dcf
popd

git clone git@github.com:p-ranav/argparse.git
pushd argparse
git checkout v2.5
popd


popd > /dev/null
