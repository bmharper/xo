#!/bin/sh
export LD_LIBRARY_PATH=$(pwd)/t2-output/linux-clang-debug-default
tundra2 linux-clang-debug-default $@ 2>&1 | sed -r -f build/linux-msvc-tx.sed
