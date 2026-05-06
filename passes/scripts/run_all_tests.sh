#!/bin/bash
set -e
source "$(dirname "$0")/common.sh"
TESTS_DIR=$ROOT/tests

echo "╔═══════════════════════════════════════════╗"
echo "║  批量多策略混淆：遍历 tests/ 下所有 .c    ║"
echo "╚═══════════════════════════════════════════╝"
echo ""

PASS=0; FAIL=0

find $TESTS_DIR -name "*.c" -type f | sort | while read SRC; do
    REL=$(realpath --relative-to=$TESTS_DIR "$SRC")
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  处理: $REL"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    if bash "$(dirname "$0")/run_both.sh" "$SRC"; then
        echo "  → ✅ 通过"
    else
        echo "  → ❌ 失败"
    fi
    echo ""
done
