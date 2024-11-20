/*
Code for accessing CUDA and other directives.
*/

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
using namespace clang::ast_matchers;

// Callback to handle matched kernel functions
class KernelPrinter : public MatchFinder::MatchCallback 
{
public:
    virtual void run(const MatchFinder::MatchResult &Result) 
    {
        if (const FunctionDecl *Func = Result.Nodes.getNodeAs<FunctionDecl>("kernelFunc")) 
        {
            if (Result.Context->getSourceManager().isInMainFile(Func->getLocation()) &&
            Func->hasAttr<CUDAGlobalAttr>() && 
            Func->hasBody()) 
            {
                llvm::outs() << "Found CUDA kernel function: " << Func->getNameAsString() << "\n";
                llvm::outs() << "Location: " << Func->getLocation().printToString(Result.Context->getSourceManager()) << "\n";
            }
        }
    }
};

class DeivcePrinter : public MatchFinder::MatchCallback 
{
public:
    virtual void run(const MatchFinder::MatchResult &Result) 
    {
        if (const FunctionDecl *Func = Result.Nodes.getNodeAs<FunctionDecl>("deviceFunc")) {
            if (Result.Context->getSourceManager().isInMainFile(Func->getLocation()) && 
            Func->hasAttr<CUDADeviceAttr>() && 
            Func->hasBody()) 
            {
                llvm::outs() << "Found CUDA device function: " << Func->getNameAsString() << "\n";
                llvm::outs() << "Location: " << Func->getLocation().printToString(Result.Context->getSourceManager()) << "\n";
            }
        }
    }
};

class HostPrinter : public MatchFinder::MatchCallback 
{
public:
    virtual void run(const MatchFinder::MatchResult &Result) 
    {
        if (const FunctionDecl *Func = Result.Nodes.getNodeAs<FunctionDecl>("hostFunc")) 
        {
            if (Result.Context->getSourceManager().isInMainFile(Func->getLocation()) && 
            Func->hasAttr<CUDAHostAttr>() && 
            Func->hasBody()) 
            {
                llvm::outs() << "Found CUDA host function: " << Func->getNameAsString() << "\n";
                llvm::outs() << "Location: " << Func->getLocation().printToString(Result.Context->getSourceManager()) << "\n";
            }
        }
    }
};


//visitor way
class FunctionVisitor : public RecursiveASTVisitor<FunctionVisitor> 
{
public:
    explicit FunctionVisitor(ASTContext *Context) : Context(Context) {}

    bool VisitFunctionDecl(FunctionDecl *F) 
    {
        if (Context->getSourceManager().isInMainFile(F->getLocation()) &&
            F->hasBody()) 
        { // Check if the function has a body
            if(F->hasAttr<CUDAHostAttr>())
            {   llvm::outs() << "Host Function name: " << F->getNameInfo().getName().getAsString()<< "\n";}
            if(F->hasAttr<CUDADeviceAttr>())
            {   llvm::outs() << "Device Function name: " << F->getNameInfo().getName().getAsString()<< "\n";}
            if(F->hasAttr<CUDAGlobalAttr>())
            {   llvm::outs() << "Kernal Function name: " << F->getNameInfo().getName().getAsString()<< "\n";}
        }
        return true; // Continue traversal
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
};

// AST Consumer to use the visitor
class FunctionASTConsumer : public ASTConsumer {
public:
    explicit FunctionASTConsumer(ASTContext *Context) : Visitor(Context) {}

    void HandleTranslationUnit(ASTContext &Context) override {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    FunctionVisitor Visitor;
};

class FunctionFrontendAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
        return std::make_unique<FunctionASTConsumer>(&CI.getASTContext());
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

    KernelPrinter kPrinter;
    DeivcePrinter dPrinter;
    HostPrinter hPrinter;
    MatchFinder Finder;
    // Matcher for CUDA kernel functions (functions with __global__ attribute)
    Finder.addMatcher(functionDecl(hasAttr(attr::CUDAGlobal)).bind("kernelFunc"), &kPrinter);
    Finder.addMatcher(functionDecl(hasAttr(attr::CUDADevice)).bind("deviceFunc"), &dPrinter);
    //Finder.addMatcher(functionDecl(hasAttr(attr::CUDAHost)).bind("hostFunc"), &hPrinter);
    Tool.run(newFrontendActionFactory<FunctionFrontendAction>().get());
    //Tool.run(newFrontendActionFactory(&Finder).get());
    return 0;
}