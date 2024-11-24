#include "clang/AST/AST.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"


#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>

using namespace clang;
using namespace clang::tooling;

class ReplacePrintfVisitor : public RecursiveASTVisitor<ReplacePrintfVisitor> {
public:
    explicit ReplacePrintfVisitor(Rewriter &R) : TheRewriter(R) {}

    bool VisitCallExpr(CallExpr *Call) {
        // Check if the function being called is "printf"
        if (const FunctionDecl *Func = Call->getDirectCallee()) {
            if (Func->getNameAsString() == "printf") {
                SourceManager &SM = TheRewriter.getSourceMgr();

                // Replace "printf(...)" with "std::cout << ..."
                std::string Replacement = "std::cout << ";
                for (unsigned i = 0; i < Call->getNumArgs(); ++i) {
                    if (i > 0) Replacement += " << ";
                    Replacement += Lexer::getSourceText(
                        CharSourceRange::getTokenRange(Call->getArg(i)->getSourceRange()), SM,
                        TheRewriter.getLangOpts())
                        .str();
                }

                TheRewriter.ReplaceText(Call->getSourceRange(), Replacement);
            }
        }
        return true;
    }

private:
    Rewriter &TheRewriter;
};

class ReplacePrintfASTConsumer : public ASTConsumer {
public:
    explicit ReplacePrintfASTConsumer(Rewriter &R) : Visitor(R) {}

    virtual void HandleTranslationUnit(ASTContext &Context) {
        // Traverse the AST
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    ReplacePrintfVisitor Visitor;
};

class ReplacePrintfFrontendAction : public ASTFrontendAction {
public:
    virtual void EndSourceFileAction() override {
        SourceManager &SM = TheRewriter.getSourceMgr();
        //llvm::outs() << "Rewriting file: " << SM.getFileEntryForID(SM.getMainFileID())->getName()<< "\n";
        llvm::outs() << "Rewriting file: " << SM.getFileEntryRefForID(SM.getMainFileID())->getName() << "\n";
        TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
    }

    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                           StringRef file) override {
        TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        return std::make_unique<ReplacePrintfASTConsumer>(TheRewriter);
    }

private:
    Rewriter TheRewriter;
};

static llvm::cl::OptionCategory ToolCategory("replace-printf options");


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
    
    
    //Tool.run(newFrontendActionFactory<FunctionFrontendAction>().get());
    Tool.run(newFrontendActionFactory<ReplacePrintfFrontendAction>().get());

    return 0;
}
