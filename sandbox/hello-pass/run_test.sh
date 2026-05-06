#!/bin/bash
set -e

# ====== 路径配置 ======
LLVM_BUILD_DIR="$HOME/graduation/build-llvm15"
PROJECT_DIR="$HOME/graduation/hello-pass"
BUILD_DIR="${PROJECT_DIR}/build"

OPT="${LLVM_BUILD_DIR}/bin/opt"
CLANG="${LLVM_BUILD_DIR}/bin/clang"

echo "============================================"
echo "  Step 1: 编译 HelloWorldPass 插件"
echo "============================================"
cd "${BUILD_DIR}"
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

if [ ! -f "HelloWorldPass.so" ]; then
    echo "[ERROR] HelloWorldPass.so 未生成！"
    exit 1
fi
echo "[OK] HelloWorldPass.so 编译成功"
echo ""

echo "============================================"
echo "  Step 2: 生成 LLVM IR"
echo "============================================"
cd "${PROJECT_DIR}"
${CLANG} -S -emit-llvm -O0 test.c -o test.ll
echo "[OK] test.ll 已生成"
echo ""

echo "============================================"
echo "  Step 3: 运行 HelloWorldPass"
echo "============================================"
${OPT} -load-pass-plugin="${BUILD_DIR}/HelloWorldPass.so" \
       -passes="hello-world" \
       -disable-output \
       test.ll

echo ""
echo "============================================"
echo "  ✅ 环境验证成功！LLVM 15 工作正常"
echo "============================================"
