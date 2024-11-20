//#include "clang/AST/AST.h"
//#include "clang/AST/ASTConsumer.h"
//#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/CompilerInstance.h"
//#include "llvm/Support/CommandLine.h"
//#include "clang/Basic/SourceManager.h"
//#include "clang/Lex/Lexer.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Rewrite/Core/Rewriter.h"

//#include "PrintProgram.h"

using namespace clang;
using namespace clang::tooling;

class FunctionVisitor : public RecursiveASTVisitor<FunctionVisitor> 
{
public:
    explicit FunctionVisitor(ASTContext *Context,Rewriter &R) : Context(Context),Rewrite(R) {}

    bool VisitFunctionDecl(FunctionDecl *F) 
    {
        if (Context->getSourceManager().isInMainFile(F->getLocation()) &&
            F->hasBody()) 
        { // Check if the function has a body
            //if(F->hasAttr<CUDAHostAttr>())
            //{   llvm::outs() << "Host Function name: " << F->getNameInfo().getName().getAsString()<< "\n";}
            //if(F->hasAttr<CUDADeviceAttr>())
            //{   llvm::outs() << "Device Function name: " << F->getNameInfo().getName().getAsString()<< "\n";}
            if(F->hasAttr<CUDAGlobalAttr>())
            {   
                llvm::outs() << "Kernal Function name: " << F->getNameInfo().getName().getAsString()<< "\n";
                kernel_transformed=false;
                //return TraverseStmt(F->getBody());
            }
        }
        return true; // Continue traversal
    }

    bool VisitCallExpr(CallExpr *Call) {
        // Check if the call is a `printf` statement
        if(!kernel_transformed)
        {
            if (const FunctionDecl *FD = Call->getDirectCallee()) {
                if (FD->getNameAsString() == "printf") {
                    llvm::outs()<<"printf found!!";
                    //Helps access each arguments of a function call
                    /*
                    SourceManager &SM = Rewrite.getSourceMgr();

                    // Replace "printf(...)" with "std::cout << ..."
                    std::string Replacement = "for(int a=0;a<10;a++)\n\t{\n\t";
                    Replacement+=FD->getNameAsString()+"(";
                    for (unsigned i = 0; i < Call->getNumArgs(); ++i) {
                        
                        if (i > 0) Replacement += ",";
                        Replacement += Lexer::getSourceText(
                            CharSourceRange::getTokenRange(Call->getArg(i)->getSourceRange()), SM,
                            Rewrite.getLangOpts())
                            .str();
                        Replacement+=");\n\t}";
                    }
                    Rewrite.ReplaceText(Call->getSourceRange(), Replacement);
                    
                    SourceLocation SemiLoc =Call->getExprStmt()->getEndLoc();
                    Rewrite.RemoveText(SemiLoc.getLocWithOffset(1), 1);
                    */

                    Rewrite.InsertText(Call->getBeginLoc(), "for (int i = 0; i < 10; ++i) {\n\t ", true, true);
                    Rewrite.InsertTextAfterToken(Call->getEndLoc(), ";\n\t}");

                    SourceLocation SemiLoc =Call->getExprStmt()->getEndLoc();
                    Rewrite.RemoveText(SemiLoc.getLocWithOffset(1), 1);

                    kernel_transformed=true;
                }
            }
        }
        return true;
    }

    bool VisitCUDAKernelCallExpr(CUDAKernelCallExpr *KernelCall) {
        // Print the function name if available
        if (const FunctionDecl *Callee = KernelCall->getDirectCallee()) {
            llvm::outs() << "Found CUDA kernel call to: " << Callee->getNameAsString() << "\n";
        }
        // Print the location of the kernel call
        //llvm::outs() << "Location: " << KernelCall->getExprLoc().printToString(Context->getSourceManager()) << "\n";
        return true; // Continue traversal
    }

private:
    ASTContext *Context;
    Rewriter &Rewrite;
    bool kernel_transformed=false;
};

// AST Consumer to use the visitor
class FunctionASTConsumer : public ASTConsumer {
public:
    explicit FunctionASTConsumer(ASTContext *Context,Rewriter &R) : Visitor(Context,R) {}

    void HandleTranslationUnit(ASTContext &Context) override {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    FunctionVisitor Visitor;
};

class FunctionFrontendAction : public ASTFrontendAction {
public:
    //used for printing the entire program
    virtual void EndSourceFileAction() override {
        SourceManager &SM = Rewriter.getSourceMgr();
        llvm::outs() << "Rewriting file: " << SM.getFileEntryRefForID(SM.getMainFileID())->getName() << "\n";
        Rewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
        Rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        return std::make_unique<FunctionASTConsumer>(&CI.getASTContext(),Rewriter);
    }
private:
    Rewriter Rewriter;
};

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
    
    
    Tool.run(newFrontendActionFactory<FunctionFrontendAction>().get());

    return 0;
}
