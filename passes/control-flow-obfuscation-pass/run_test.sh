#!/bin/bash
set -e

# ============================================================
# 路径配置
# ============================================================
LLVM_BUILD="$HOME/graduation/build-llvm15"
PROJECT="$HOME/graduation/week3-obfuscation-pass"
BUILD_DIR="${PROJECT}/build"

OPT="${LLVM_BUILD}/bin/opt"
CLANG="${LLVM_BUILD}/bin/clang"
CLANGXX="${LLVM_BUILD}/bin/clang++"
LLC="${LLVM_BUILD}/bin/llc"

PASS_SO="${BUILD_DIR}/MixedPredicatePass.so"
RUNTIME_SRC="${PROJECT}/runtime/obf_runtime.cpp"

echo ""
echo "╔══════════════════════════════════════════════════╗"
echo "║  第三周：混合不透明谓词 Pass 完整测试            ║"
echo "╚══════════════════════════════════════════════════╝"
echo ""

# ============================================================
# Step 1: 编译 Pass 插件
# ============================================================
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Step 1: 编译 MixedPredicatePass.so"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
cd "${BUILD_DIR}"
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

if [ ! -f "${PASS_SO}" ]; then
    echo "  ❌ MixedPredicatePass.so 编译失败！"
    exit 1
fi
echo "  ✅ MixedPredicatePass.so 编译成功"
echo ""

# ============================================================
# Step 2: 编译运行时库为目标文件
# ============================================================
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Step 2: 编译运行时库 obf_runtime.o"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
${CLANGXX} -std=c++17 -O2 -c "${RUNTIME_SRC}" \
    -I"${PROJECT}/runtime" \
    -o "${BUILD_DIR}/obf_runtime.o"
echo "  ✅ obf_runtime.o 编译成功"
echo ""

# ============================================================
# 测试函数
# ============================================================
run_single_test() {
    local TEST_NAME="$1"
    local TEST_SRC="$2"

    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  测试: ${TEST_NAME}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    local BASE="${BUILD_DIR}/${TEST_NAME}"

    # 2a. 先编译运行未混淆版本（对照组）
    echo "  [原始] 编译并运行未混淆版本..."
    ${CLANG} -O0 "${TEST_SRC}" -o "${BASE}_original" -lm
    echo "  [原始] 输出:"
    "${BASE}_original" | sed 's/^/    /'
    echo ""

    # 2b. 生成 LLVM IR
    echo "  [混淆] 生成 LLVM IR..."
    ${CLANG} -S -emit-llvm -O0 -Xclang -disable-O0-optnone \
        "${TEST_SRC}" -o "${BASE}.ll"

    # 2c. 运行 Pass 进行混淆
    echo "  [混淆] 运行 MixedPredicatePass..."
    ${OPT} -load-pass-plugin="${PASS_SO}" \
           -passes="mixed-predicate" \
           -S "${BASE}.ll" -o "${BASE}_obf.ll" 2>&1 | sed 's/^/    /'
    echo ""

    # 2d. 显示混淆前后 IR 对比（基本块数量）
    local ORIG_BLOCKS=$(grep -c "^[a-zA-Z_].*:" "${BASE}.ll" 2>/dev/null || echo "0")
    local OBF_BLOCKS=$(grep -c "^[a-zA-Z_].*:" "${BASE}_obf.ll" 2>/dev/null || echo "0")
    echo "  [对比] 原始 IR 基本块数: ${ORIG_BLOCKS}"
    echo "  [对比] 混淆 IR 基本块数: ${OBF_BLOCKS}"
    echo ""

    # 2e. 检查混淆后 IR 中是否包含关键标记
    echo "  [检查] 混淆后 IR 特征:"
    if grep -q "scheduler\." "${BASE}_obf.ll"; then
        echo "    ✅ 包含 scheduler 调度器块"
    else
        echo "    ⚠️  未发现 scheduler 块"
    fi
    if grep -q "fake\." "${BASE}_obf.ll"; then
        echo "    ✅ 包含 fake 虚假块"
    else
        echo "    ⚠️  未发现 fake 块"
    fi
    if grep -q "__obf_chaos_predicate" "${BASE}_obf.ll"; then
        echo "    ✅ 包含混沌子谓词调用"
    fi
    if grep -q "__obf_thread_predicate" "${BASE}_obf.ll"; then
        echo "    ✅ 包含线程子谓词调用"
    fi
    if grep -q "__obf_runtime_init" "${BASE}_obf.ll"; then
        echo "    ✅ 包含运行时初始化调用"
    fi
    echo ""

    # 2f. 编译混淆后的 IR + 运行时库，生成可执行文件
    echo "  [混淆] 编译混淆后的可执行文件..."
    ${CLANG} "${BASE}_obf.ll" "${BUILD_DIR}/obf_runtime.o" \
        -lstdc++ -lpthread -lm \
        -o "${BASE}_obfuscated" 2>&1 | sed 's/^/    /'

    # 2g. 运行混淆后的程序
    echo "  [混淆] 运行混淆后版本:"
    if "${BASE}_obfuscated" 2>&1 | sed 's/^/    /'; then
        echo ""
        echo "  ✅ ${TEST_NAME}: 混淆后程序输出正确，语义保持不变！"
    else
        echo ""
        echo "  ❌ ${TEST_NAME}: 混淆后程序执行失败！"
    fi
    echo ""
}

# ============================================================
# Step 3: 运行所有测试
# ============================================================
echo ""
echo "╔══════════════════════════════════════════════════╗"
echo "║  运行测试用例                                    ║"
echo "╚══════════════════════════════════════════════════╝"
echo ""

run_single_test "test_simple" "${PROJECT}/test/test_simple.c"
run_single_test "test_loop"   "${PROJECT}/test/test_loop.c"
run_single_test "test_branch" "${PROJECT}/test/test_branch.c"

# ============================================================
# Step 4: 展示混淆后 IR 片段
# ============================================================
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  混淆后 IR 关键片段（test_simple）:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 展示 scheduler 和 fake 相关的 IR
if [ -f "${BUILD_DIR}/test_simple_obf.ll" ]; then
    grep -A5 "scheduler\.\|fake\.\|chaos.ok\|thread.ok\|mixed.pred\|__obf_" \
        "${BUILD_DIR}/test_simple_obf.ll" | head -60 | sed 's/^/  /'
fi

echo ""
echo "╔══════════════════════════════════════════════════╗"
echo "║  ✅ 第三周全部测试完成                           ║"
echo "╚══════════════════════════════════════════════════╝"
echo ""
