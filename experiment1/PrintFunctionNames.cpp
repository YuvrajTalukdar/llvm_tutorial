#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// A function pass that prints the name of each function in the module
struct FunctionPrinterPass : public PassInfoMixin<FunctionPrinterPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
        // Print a debug message before printing function names
        outs() << "Running FunctionPrinterPass on function: " << F.getName() << "\n";
        outs().flush();
        return PreservedAnalyses::all();
    }
};

// This is the entry point for the pass plugin
PassPluginLibraryInfo getPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "FunctionPrinterPass", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>) {
                        if (Name == "print-function-names") {
                            MPM.addPass(createModuleToFunctionPassAdaptor(FunctionPrinterPass()));
                            return true;
                        }
                        return false;
                    });
            }};
}

// Extern "C" ensures this function can be called when the pass is loaded
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getPassPluginInfo();
}
