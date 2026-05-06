#!/bin/bash
set -e
source "$(dirname "$0")/common.sh"
SRC=$1
[ -z "$SRC" ] && { echo "用法: $0 <test.c>"; exit 1; }

NAME=$(prepare_baseline "$SRC")

echo ""
echo "========== 单独控制流混淆（unit-test） =========="
echo "[原始 IR]  $DIR_NON_IR/$NAME.ll"

$OPT -load-pass-plugin=$CF_SO -passes="control-flow-obf" \
     -S $DIR_NON_IR/$NAME.ll -o $DIR_UNIT/$NAME.cf.ll 2>&1

echo "[混淆 IR]  $DIR_UNIT/$NAME.cf.ll"

$CLANG $DIR_UNIT/$NAME.cf.ll $RT_DIR/obf_runtime.o \
       -lstdc++ -lpthread -lm -o $DIR_UNIT/$NAME.cf

echo "[可执行]   $DIR_UNIT/$NAME.cf"
echo ""
echo "===== 原始输出 ====="
$DIR_NON_ELF/$NAME
echo ""
echo "===== 混淆后输出 ====="
$DIR_UNIT/$NAME.cf
