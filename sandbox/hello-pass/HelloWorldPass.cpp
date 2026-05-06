#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

struct HelloWorldPass : public PassInfoMixin<HelloWorldPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        errs() << "[HelloWorldPass] Found function: " << F.getName() << "\n";
        errs() << "  - Number of basic blocks: " << F.size() << "\n";
        errs() << "  - Number of instructions: " << F.getInstructionCount() << "\n";
        return PreservedAnalyses::all();
    }

    static bool isRequired() { return true; }
};

} // anonymous namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "HelloWorldPass",
        .PluginVersion = LLVM_VERSION_STRING,
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "hello-world") {
                        FPM.addPass(HelloWorldPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}
