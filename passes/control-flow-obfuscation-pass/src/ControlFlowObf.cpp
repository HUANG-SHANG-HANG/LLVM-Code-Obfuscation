// 控制流混淆 Pass：在每个基本块前插入 scheduler + fake_block
// scheduler 使用 (chaos_ok || thread_ok) 决定真实跳转方向
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
#include <random>
using namespace llvm;


namespace {
static FunctionCallee getChaos(Module &M)   { return M.getOrInsertFunction("__obf_chaos_predicate",  FunctionType::get(Type::getInt1Ty(M.getContext()),false)); }
static FunctionCallee getThread(Module &M)  { return M.getOrInsertFunction("__obf_thread_predicate", FunctionType::get(Type::getInt1Ty(M.getContext()),false)); }
static FunctionCallee getInit(Module &M)    { return M.getOrInsertFunction("__obf_runtime_init",     FunctionType::get(Type::getVoidTy(M.getContext()),false)); }


static void fillFake(BasicBlock *BB, LLVMContext &C) {
    IRBuilder<> B(BB);
    static std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<int> d(1, 255);
    Value *v1 = ConstantInt::get(Type::getInt32Ty(C), d(rng));
    Value *v2 = ConstantInt::get(Type::getInt32Ty(C), d(rng));
    Value *x  = B.CreateXor(v1, v2, "fake.xor");
    Value *a  = B.CreateAdd(x, v1, "fake.add");
    B.CreateShl(a, ConstantInt::get(Type::getInt32Ty(C), 2), "fake.shl");
}


struct ControlFlowObfPass : public PassInfoMixin<ControlFlowObfPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        if (F.isDeclaration()) return PreservedAnalyses::all();
        StringRef N = F.getName();
        if (N.startswith("__obf_")) return PreservedAnalyses::all();
        Module &M = *F.getParent(); LLVMContext &C = M.getContext();


        // 入口块插入 runtime_init
        IRBuilder<> EB(&*F.getEntryBlock().getFirstInsertionPt());
        EB.CreateCall(getInit(M));


        // 收集需混淆的基本块
        std::vector<BasicBlock*> targets;
        for (BasicBlock &BB : F) {
            if (&BB == &F.getEntryBlock()) continue;
            if (!BB.getTerminator()) continue;
            if (BB.size() < 2) continue;
            targets.push_back(&BB);
        }


        int n = 0;
        for (BasicBlock *BB : targets) {
            BasicBlock *Sched = BasicBlock::Create(C, "sched."+BB->getName(), &F, BB);
            BasicBlock *Fake  = BasicBlock::Create(C, "fake." +BB->getName(), &F, BB);


            IRBuilder<> SB(Sched);
            Value *ck = SB.CreateCall(getChaos(M),  {}, "chaos.ok");
            Value *tk = SB.CreateCall(getThread(M), {}, "thread.ok");
            Value *mx = SB.CreateOr(ck, tk, "mixed");
            SB.CreateCondBr(mx, BB, Fake);


            fillFake(Fake, C);
            IRBuilder<> FB(Fake);
            FB.CreateBr(BB);


            // 重定向所有前驱
            std::vector<BasicBlock*> preds;
            for (BasicBlock *P : predecessors(BB))
                if (P != Sched && P != Fake) preds.push_back(P);
            for (BasicBlock *P : preds) {
                Instruction *T = P->getTerminator();
                for (unsigned i=0; i<T->getNumSuccessors(); ++i)
                    if (T->getSuccessor(i)==BB) T->setSuccessor(i, Sched);
            }
            // 处理 PHI
            for (PHINode &Phi : BB->phis()) {
                Phi.addIncoming(UndefValue::get(Phi.getType()), Sched);
                Phi.addIncoming(UndefValue::get(Phi.getType()), Fake);
            }
            ++n;
        }
        errs() << "[CF-Obf] " << F.getName() << ": obfuscated " << n << " blocks\n";
        return n ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
    static bool isRequired() { return true; }
};
}


extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return { LLVM_PLUGIN_API_VERSION, "ControlFlowObfPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef N, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (N == "control-flow-obf") { FPM.addPass(ControlFlowObfPass()); return true; }
                    return false;
                });
        }};
}
