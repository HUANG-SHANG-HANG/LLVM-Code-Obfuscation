#!/bin/bash
export ROOT=/home/hang/graduationDesign
export LLVM_BIN=$ROOT/toolChain/build-llvm15/bin
export CLANG=$LLVM_BIN/clang
export CLANGXX=$LLVM_BIN/clang++
export OPT=$LLVM_BIN/opt

export PASSES=$ROOT/passes
export RT_DIR=$PASSES/runtime
export CF_SO=$PASSES/control-flow-obfuscation-pass/build/ControlFlowObfPass.so
export ARR_SO=$PASSES/array-obfuscation-pass/build/ArrayObfPass.so

export OUT=$ROOT/tests-outputs
export DIR_NON_IR=$OUT/non-obf-ir
export DIR_NON_ELF=$OUT/non-obf-elf
export DIR_OBF_IR=$OUT/obf-ir
export DIR_OBF_ELF=$OUT/obf-elf
export DIR_UNIT=$OUT/unit-test-output

mkdir -p $DIR_NON_IR $DIR_NON_ELF $DIR_OBF_IR $DIR_OBF_ELF $DIR_UNIT

# 生成原始 IR + 原始可执行文件
prepare_baseline() {
    local SRC=$1
    local NAME=$(basename "$SRC" .c)

    # 原始 IR
    $CLANG -S -emit-llvm -O0 -Xclang -disable-O0-optnone \
           "$SRC" -o $DIR_NON_IR/$NAME.ll

    # 原始可执行文件（对照组）
    $CLANG -O0 "$SRC" -o $DIR_NON_ELF/$NAME -lm

    echo "$NAME"
}
