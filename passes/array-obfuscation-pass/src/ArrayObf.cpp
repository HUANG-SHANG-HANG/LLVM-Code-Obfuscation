// 数组混淆 Pass：将所有数组 GEP 的可变索引包装为 __obf_array_idx(i)
// 该函数运行时返回 i 本身，但中间过程含 XOR/加减，使静态分析难以追踪
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
using namespace llvm;


namespace {
static FunctionCallee getIdx(Module &M) {
    LLVMContext &C = M.getContext();
    return M.getOrInsertFunction("__obf_array_idx",
        FunctionType::get(Type::getInt64Ty(C), {Type::getInt64Ty(C)}, false));
}


struct ArrayObfPass : public PassInfoMixin<ArrayObfPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        if (F.isDeclaration()) return PreservedAnalyses::all();
        if (F.getName().startswith("__obf_")) return PreservedAnalyses::all();


        Module &M = *F.getParent(); LLVMContext &C = M.getContext();
        std::vector<GetElementPtrInst*> targets;


        for (auto &BB : F)
            for (auto &I : BB)
                if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
                    // 只处理源类型是数组的 GEP
                    if (!GEP->getSourceElementType()->isArrayTy()) continue;
                    targets.push_back(GEP);
                }


        int n = 0;
        FunctionCallee Idx = getIdx(M);


        for (auto *GEP : targets) {
            // 找最后一个非常量索引（数组下标）
            unsigned numOp = GEP->getNumIndices();
            if (numOp < 2) continue; // 至少两个索引（第一个是0，第二个才是元素）
            unsigned last = GEP->getNumOperands() - 1;
            Value *idx = GEP->getOperand(last);
            if (isa<ConstantInt>(idx)) continue; // 跳过常量索引


            IRBuilder<> B(GEP);
            // 扩展到 i64
            Value *i64 = idx->getType()->isIntegerTy(64)
                       ? idx : B.CreateSExt(idx, Type::getInt64Ty(C), "idx.s64");
            // 调用恒等加密函数
            Value *enc = B.CreateCall(Idx, {i64}, "idx.obf");
            // 截回原类型
            Value *back = enc->getType() == idx->getType()
                       ? enc : B.CreateTrunc(enc, idx->getType(), "idx.tr");
            GEP->setOperand(last, back);
            ++n;
        }


        errs() << "[Array-Obf] " << F.getName() << ": rewrote " << n << " GEPs\n";
        return n ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
    static bool isRequired() { return true; }
};
}


extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return { LLVM_PLUGIN_API_VERSION, "ArrayObfPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef N, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (N == "array-obf") { FPM.addPass(ArrayObfPass()); return true; }
                    return false;
                });
        }};
}
