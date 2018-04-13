#!/bin/bash

cd "$WORKDIR"
CMAKE_BUILD_DIR=build

if [ ! -d "$CMAKE_BUILD_DIR" ]; then
  mkdir -p "$CMAKE_BUILD_DIR";
fi

cd "$CMAKE_BUILD_DIR"
cmake ..
make -j $@

