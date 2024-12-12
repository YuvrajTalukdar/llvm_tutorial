#include "clang/Analysis/CallGraph.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include <iostream>

using namespace clang;
using namespace clang::tooling;
using namespace std;

class CallGraphConsumer : public ASTConsumer {
public:
    explicit CallGraphConsumer(ASTContext &Context) : Context(Context) {}

    void HandleTranslationUnit(ASTContext &Context) override {
        CallGraph CG;//(Context);
        CG.addToCallGraph(Context.getTranslationUnitDecl());

        // Traverse and print the call graph
        for (auto it = CG.begin(); it != CG.end(); ++it) {
            const Decl *callerDecl = it->first;
            if (!callerDecl) continue; // Skip null nodes

            const auto *callerFD = dyn_cast<FunctionDecl>(callerDecl);
            if (!callerFD || !callerFD->hasBody()) continue;

            std::string callerName = callerFD->getNameAsString();
            std::cout << callerName << " calls:\n";

            for (const CallGraphNode *calleeNode : *(it->second)) {
                if (const auto *calleeFD = dyn_cast_or_null<FunctionDecl>(calleeNode->getDecl())) {
                    std::string calleeName = calleeFD->getNameAsString();
                    std::cout << "  " << calleeName << "\n";
                }
            }
        }
    }

private:
    ASTContext &Context;
};

class CallGraphAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef File) override {
        return std::make_unique<CallGraphConsumer>(CI.getASTContext());
    }
};

int main(int argc, const char **argv) {
    llvm::cl::OptionCategory MyToolCategory("my-tool options");
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << "Error: " << llvm::toString(ExpectedParser.takeError()) << "\n";
        return 1;
    }

    CommonOptionsParser &OptionsParser = *ExpectedParser;
    const auto &files = OptionsParser.getSourcePathList();
    if (files.size() != 1) {
        llvm::errs() << "Expected exactly 1 files: main file and function file.\n";
        return 1;
    }

    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("--cuda-gpu-arch=sm_86", ArgumentInsertPosition::END));  // Specify CUDA architecture
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-resource-dir", ArgumentInsertPosition::END));
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("/usr/lib/clang/18", ArgumentInsertPosition::END));
    Tool.run(newFrontendActionFactory<CallGraphAction>().get());

    return 0;
}
