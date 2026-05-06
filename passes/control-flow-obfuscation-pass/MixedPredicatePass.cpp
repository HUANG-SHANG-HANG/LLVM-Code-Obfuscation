///////////////////////////////////////////////////////////////////////////////
// MixedPredicatePass.cpp
// 第三周任务二：混合不透明谓词 LLVM Pass
//
// 功能：
//   1. 遍历函数中所有基本块，识别 BranchInst
//   2. 对每个可处理的基本块，执行控制流重构：
//      - 在原始块之前插入 scheduler 调度器块
//      - 在 scheduler 中插入混合谓词（call + or + br）
//      - 插入 fake_block 虚假块
//   3. 在函数入口插入 __obf_runtime_init() 调用
//   4. 保证语义正确：无论谓词结果如何，程序都会执行原始逻辑
//
// 使用方式（Out-of-Tree，通过 opt 加载）：
//   opt -load-pass-plugin=./MixedPredicatePass.so \
//       -passes="mixed-predicate" -S test.ll -o test_obf.ll
///////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <vector>
#include <random>

using namespace llvm;

namespace {

// ============================================================
// 辅助函数：获取或声明运行时函数
// ============================================================

/// 获取或创建函数声明：bool __obf_chaos_predicate(void)
static FunctionCallee getOrInsertChaosPredicate(Module &M) {
    LLVMContext &Ctx = M.getContext();
    FunctionType *FTy = FunctionType::get(
        Type::getInt1Ty(Ctx),   // 返回 i1 (bool)
        false                   // 无参数
    );
    return M.getOrInsertFunction("__obf_chaos_predicate", FTy);
}

/// 获取或创建函数声明：bool __obf_thread_predicate(void)
static FunctionCallee getOrInsertThreadPredicate(Module &M) {
    LLVMContext &Ctx = M.getContext();
    FunctionType *FTy = FunctionType::get(
        Type::getInt1Ty(Ctx),
        false
    );
    return M.getOrInsertFunction("__obf_thread_predicate", FTy);
}

/// 获取或创建函数声明：void __obf_runtime_init(void)
static FunctionCallee getOrInsertRuntimeInit(Module &M) {
    LLVMContext &Ctx = M.getContext();
    FunctionType *FTy = FunctionType::get(
        Type::getVoidTy(Ctx),
        false
    );
    return M.getOrInsertFunction("__obf_runtime_init", FTy);
}

// ============================================================
// 辅助函数：生成虚假块中的迷惑性指令
// ============================================================

/// 在 fake_block 中插入看起来有意义但实际无用的指令
/// 目的：干扰反编译器的 CFG 分析，使 fake_block 看起来像真实代码
static void insertFakeInstructions(BasicBlock *FakeBB, LLVMContext &Ctx) {
    IRBuilder<> Builder(FakeBB);

    // 生成一些迷惑性的算术运算
    // 使用伪随机数让每个 fake_block 看起来不同
    static std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, 255);

    Value *V1 = ConstantInt::get(Type::getInt32Ty(Ctx), dist(rng));
    Value *V2 = ConstantInt::get(Type::getInt32Ty(Ctx), dist(rng));

    // 异或运算（常见的混淆手法）
    Value *Xor = Builder.CreateXor(V1, V2, "fake.xor");

    // 加法
    Value *Add = Builder.CreateAdd(Xor, V1, "fake.add");

    // 左移
    Value *Shl = Builder.CreateShl(Add,
        ConstantInt::get(Type::getInt32Ty(Ctx), 2), "fake.shl");

    // 比较（结果不使用，纯粹迷惑）
    Builder.CreateICmpEQ(Shl, V2, "fake.cmp");

    // 注意：此处不插入 terminator，由调用者负责添加 br 指令
}

// ============================================================
// 核心 Pass 实现
// ============================================================

struct MixedPredicatePass : public PassInfoMixin<MixedPredicatePass> {

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        // 跳过外部函数声明
        if (F.isDeclaration()) {
            return PreservedAnalyses::all();
        }

        // 跳过运行时库自身的函数（防止递归混淆）
        StringRef Name = F.getName();
        if (Name.startswith("__obf_") || Name == "guardian_thread_func") {
            return PreservedAnalyses::all();
        }

        Module &M = *F.getParent();
        LLVMContext &Ctx = M.getContext();

        errs() << "[MixedPredicatePass] Processing function: "
               << F.getName() << "\n";

        // ---- Step 1: 在函数入口插入 runtime_init 调用 ----
        insertRuntimeInitCall(F, M);

        // ---- Step 2: 收集需要处理的基本块 ----
        // 注意：不能在遍历过程中修改 BB 列表，先收集再处理
        std::vector<BasicBlock *> BlocksToProcess;

        for (BasicBlock &BB : F) {
            // 跳过入口块（entry block）—— 因为我们已在其中插入了 init 调用
            if (&BB == &F.getEntryBlock()) {
                continue;
            }

            // 只处理有 terminator 的块
            Instruction *Term = BB.getTerminator();
            if (!Term) continue;

            // 只处理包含多条指令的有意义的块（跳过空块）
            if (BB.size() < 2) continue;

            BlocksToProcess.push_back(&BB);
        }

        if (BlocksToProcess.empty()) {
            errs() << "  No blocks to obfuscate.\n";
            return PreservedAnalyses::all();
        }

        // ---- Step 3: 对每个块执行控制流重构 ----
        int obfuscatedCount = 0;
        for (BasicBlock *BB : BlocksToProcess) {
            if (obfuscateBlock(BB, M, Ctx)) {
                obfuscatedCount++;
            }
        }

        errs() << "  Obfuscated " << obfuscatedCount << " blocks in "
               << F.getName() << "\n";

        if (obfuscatedCount > 0) {
            return PreservedAnalyses::none();
        }
        return PreservedAnalyses::all();
    }

    static bool isRequired() { return true; }

private:

    /// 在函数入口的第一条指令前插入 __obf_runtime_init() 调用
    void insertRuntimeInitCall(Function &F, Module &M) {
        BasicBlock &EntryBB = F.getEntryBlock();
        IRBuilder<> Builder(&*EntryBB.getFirstInsertionPt());

        FunctionCallee InitFunc = getOrInsertRuntimeInit(M);
        Builder.CreateCall(InitFunc);

        errs() << "  Inserted __obf_runtime_init() at entry of "
               << F.getName() << "\n";
    }

    /// 对单个基本块执行控制流重构
    /// 返回 true 表示成功混淆
    bool obfuscateBlock(BasicBlock *OrigBB, Module &M, LLVMContext &Ctx) {
        Function *F = OrigBB->getParent();
        Instruction *Term = OrigBB->getTerminator();

        // ---- 创建 scheduler 调度器块 ----
        BasicBlock *SchedulerBB = BasicBlock::Create(
            Ctx, "scheduler." + OrigBB->getName(), F, OrigBB);

        // ---- 创建 fake_block 虚假块 ----
        BasicBlock *FakeBB = BasicBlock::Create(
            Ctx, "fake." + OrigBB->getName(), F, OrigBB);

        // ---- 填充 scheduler 块 ----
        IRBuilder<> SchedBuilder(SchedulerBB);

        // 调用混沌子谓词
        FunctionCallee ChaosFunc = getOrInsertChaosPredicate(M);
        Value *ChaosOk = SchedBuilder.CreateCall(ChaosFunc, {}, "chaos.ok");

        // 调用线程子谓词
        FunctionCallee ThreadFunc = getOrInsertThreadPredicate(M);
        Value *ThreadOk = SchedBuilder.CreateCall(ThreadFunc, {}, "thread.ok");

        // OR 混合：mixed = chaos_ok || thread_ok
        Value *MixedPred = SchedBuilder.CreateOr(ChaosOk, ThreadOk, "mixed.pred");

        // 条件分支：true → 原始块，false → 虚假块
        SchedBuilder.CreateCondBr(MixedPred, OrigBB, FakeBB);

        // ---- 填充 fake_block ----
        insertFakeInstructions(FakeBB, Ctx);

        // fake_block 最终跳回原始块（保证语义正确）
        IRBuilder<> FakeBuilder(FakeBB);
        FakeBuilder.CreateBr(OrigBB);

        // ---- 重定向前驱块的跳转 ----
        // 将所有原本跳转到 OrigBB 的前驱块改为跳转到 SchedulerBB
        std::vector<BasicBlock *> Predecessors;
        for (BasicBlock *Pred : predecessors(OrigBB)) {
            // 不要重定向我们刚刚创建的块
            if (Pred == SchedulerBB || Pred == FakeBB) continue;
            Predecessors.push_back(Pred);
        }

        for (BasicBlock *Pred : Predecessors) {
            Instruction *PredTerm = Pred->getTerminator();
            if (!PredTerm) continue;

            // 替换 terminator 中所有对 OrigBB 的引用为 SchedulerBB
            for (unsigned i = 0, e = PredTerm->getNumSuccessors(); i < e; ++i) {
                if (PredTerm->getSuccessor(i) == OrigBB) {
                    PredTerm->setSuccessor(i, SchedulerBB);
                }
            }
        }

        // ---- 处理 PHI 节点 ----
        // 如果 OrigBB 中有 PHI 节点，需要将来自旧前驱的 incoming block
        // 更新为 SchedulerBB 和 FakeBB
        for (PHINode &Phi : OrigBB->phis()) {
            // 为来自 SchedulerBB 和 FakeBB 的边添加 incoming value
            // 使用 Undef 值（因为这些是新增的控制流边）
            // 实际上 scheduler 和 fake 不产生新值给 PHI，
            // 所以需要将原来的 incoming block 改为 scheduler
            for (unsigned i = 0, e = Phi.getNumIncomingValues(); i < e; ++i) {
                BasicBlock *InBB = Phi.getIncomingBlock(i);
                // 如果原来的前驱已被重定向到 scheduler
                // PHI 中对应的 incoming block 也要改
                bool redirected = false;
                for (BasicBlock *P : Predecessors) {
                    if (InBB == P) {
                        redirected = true;
                        break;
                    }
                }
                // 注意：SchedulerBB 和 FakeBB 都可能跳到 OrigBB
                // 但它们不产生新值，所以 PHI 需要特殊处理
            }
            // 为 scheduler → OrigBB 的边添加 incoming
            Phi.addIncoming(UndefValue::get(Phi.getType()), SchedulerBB);
            // 为 fake → OrigBB 的边添加 incoming
            Phi.addIncoming(UndefValue::get(Phi.getType()), FakeBB);
        }

        errs() << "    Obfuscated block: " << OrigBB->getName()
               << " (added scheduler + fake)\n";

        return true;
    }
};

} // anonymous namespace

// ============================================================
// Pass 插件注册
// ============================================================

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "MixedPredicatePass",
        .PluginVersion = LLVM_VERSION_STRING,
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "mixed-predicate") {
                        FPM.addPass(MixedPredicatePass());
                        return true;
                    }
                    return false;
                });
        }
    };
}
