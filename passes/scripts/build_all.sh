#!/bin/bash
set -e
source "$(dirname "$0")/common.sh"

echo "=========================================="
echo "  编译 runtime"
echo "=========================================="
bash $RT_DIR/build.sh

for D in control-flow-obfuscation-pass array-obfuscation-pass; do
    echo ""
    echo "=========================================="
    echo "  编译 $D"
    echo "=========================================="
    mkdir -p $PASSES/$D/build
    cd $PASSES/$D/build
    cmake .. -DCMAKE_BUILD_TYPE=Release > /dev/null
    make -j$(nproc)
done

echo ""
echo "=========================================="
echo "  全部编译完成"
echo "  CF  Pass : $CF_SO"
echo "  Arr Pass : $ARR_SO"
echo "  Runtime  : $RT_DIR/*.o"
echo "=========================================="
