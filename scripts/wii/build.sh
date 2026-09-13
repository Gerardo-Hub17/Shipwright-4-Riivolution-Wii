#!/bin/bash
set -e

export DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
export DEVKITPPC=${DEVKITPPC:-$DEVKITPRO/devkitPPC}

cmake -S . -B build-wii -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Wii.cmake \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-wii --target soh
