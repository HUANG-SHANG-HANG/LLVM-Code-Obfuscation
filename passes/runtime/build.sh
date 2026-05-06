#!/bin/bash
set -e
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CLANGXX=/home/hang/graduationDesign/toolChain/build-llvm15/bin/clang++
$CLANGXX -std=c++17 -O2 -fPIC -c $DIR/obf_runtime.cpp   -o $DIR/obf_runtime.o
$CLANGXX -std=c++17 -O2 -fPIC -c $DIR/array_runtime.cpp -o $DIR/array_runtime.o
echo "[OK] runtime: obf_runtime.o + array_runtime.o"
