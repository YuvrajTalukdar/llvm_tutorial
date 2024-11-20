

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "PrintProgram.h"

using namespace clang::tooling;

int main(int argc, const char **argv) {
    llvm::cl::OptionCategory MyToolCategory("my-tool options");
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << "Error: " << llvm::toString(ExpectedParser.takeError()) << "\n";
        return 1;
    }

    CommonOptionsParser &OptionsParser = *ExpectedParser;
    //CommonOptionsParser OptionsParser(argc, argv, llvm::cl::GeneralCategory);
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

    // Add the necessary flags for CUDA support.
    //Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-v", ArgumentInsertPosition::BEGIN));
    
    // parsing flags for clang++

    //Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("--cuda-path=/opt/cuda", ArgumentInsertPosition::END));    // CUDA path
    //Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-I/opt/cuda/include", ArgumentInsertPosition::END));       // Include paths for CUDA
    //Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-L/opt/cuda/lib64/libcudart.so", ArgumentInsertPosition::END));        // Library paths for CUDA

    //enable this to make it work (No errors here)
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("--cuda-gpu-arch=sm_86", ArgumentInsertPosition::END));  // Specify CUDA architecture
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-resource-dir", ArgumentInsertPosition::END));
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("/usr/lib/clang/18", ArgumentInsertPosition::END));

    //enable this to make it work (Some errors here)
    //Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-I/usr/lib/clang/18/include/", ArgumentInsertPosition::END));
    //Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-Xclang", ArgumentInsertPosition::END));
    //Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-fno-cuda-host-device-constexpr", ArgumentInsertPosition::END));

    return Tool.run(newFrontendActionFactory<SourcePrinterAction>().get());
}