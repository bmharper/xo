#!/bin/sh
export LD_LIBRARY_PATH=$(pwd)/t2-output/linux-gcc-debug-default
tundra2 linux-gcc-debug-default $@ 2>&1 | sed -r -f build/linux-msvc-tx.sed
