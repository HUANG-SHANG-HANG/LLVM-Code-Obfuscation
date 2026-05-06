#!/bin/bash
set -e
source "$(dirname "$0")/common.sh"
SRC=$1
[ -z "$SRC" ] && { echo "用法: $0 <test.c>"; exit 1; }

NAME=$(prepare_baseline "$SRC")
echo "──────────────────────── 未混淆的中间代码IR和可执行文件ELF存放位置 ────────────────────────"

echo ""
echo "[未混淆 IR  存入]  $DIR_NON_IR/$NAME.ll"
echo "[未混淆 ELF 存入]  $DIR_NON_ELF/$NAME"
echo ""

echo "──────────────────────── 混淆的中间代码IR和可执行文件ELF存放位置 ────────────────────────"

echo ""
echo "[混淆 IR  存入]   $DIR_OBF_IR/$NAME.ll"
echo "[混淆 ELF 存入]  $DIR_OBF_ELF/$NAME"
echo ""

echo "──────────────────────── 语义正确性验证 ────────────────────────"
echo ""
echo ">>> 原始程序输出:"
$DIR_NON_ELF/$NAME
echo ""
echo ">>> 混淆后程序输出:"
$DIR_OBF_ELF/$NAME
echo ""

# 自动对比
ORIG=$($DIR_NON_ELF/$NAME 2>&1)
OBFD=$($DIR_OBF_ELF/$NAME 2>&1)
if [ "$ORIG" = "$OBFD" ]; then
    echo " 语义验证通过：可执行文件执行结果输出一致"
else
    echo " 语义不一致！请检查"
    diff <(echo "$ORIG") <(echo "$OBFD") || true
fi

echo ""

echo "──────────────────────── IR 复杂度对比 ────────────────────────"
echo ""
ORIG_BB=$(grep -c '^[a-zA-Z_%.][^:]*:' $DIR_NON_IR/$NAME.ll 2>/dev/null || echo 0)
OBF_BB=$(grep -c  '^[a-zA-Z_%.][^:]*:' $DIR_OBF_IR/$NAME.ll 2>/dev/null || echo 0)
ORIG_LINES=$(wc -l < $DIR_NON_IR/$NAME.ll)
OBF_LINES=$(wc -l  < $DIR_OBF_IR/$NAME.ll)
echo "  原始 IR : ${ORIG_BB} 基本块, ${ORIG_LINES} 行"
echo "  混淆 IR : ${OBF_BB} 基本块, ${OBF_LINES} 行"
echo "  基本块膨胀 : $(echo "scale=1; $OBF_BB * 100 / $ORIG_BB" | bc)%"
echo "  行数膨胀   : $(echo "scale=1; $OBF_LINES * 100 / $ORIG_LINES" | bc)%"
