#include "clang/Frontend/CompilerInstance.h"

#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "clang/AST/ASTTypeTraits.h" //#include "clang/AST/DynTypedNode.h"
#include "clang/AST/ParentMapContext.h"

using namespace clang;
using namespace clang::tooling;
using namespace std;

class FunctionVisitor : public RecursiveASTVisitor<FunctionVisitor> 
{
public:
    explicit FunctionVisitor(ASTContext *Context,Rewriter &R) : Context(Context),Rewrite(R) {}

    bool VisitStmt(Stmt *S) 
    {
        // Get the declaration that this expression refers to
        bool Result;
        if (auto *CallExprNode = dyn_cast<CallExpr>(S)) 
        {
            if(const FunctionDecl *FD = CallExprNode->getDirectCallee())
            {
                if (FD->getNameAsString() == "foo109") 
                {
                    llvm::outs()<<"foo107 found!! ";
                    DynTypedNode currentNode = Context->getParents(*CallExprNode)[0];
                    while (true) 
                    {
                        const DynTypedNodeList &parents = Context->getParents(currentNode);
                        if (parents.empty()) {
                            llvm::errs() << "No more parents.\n";
                            break;
                        }

                        // Process each parent (usually there's just one)
                        const auto &parent = parents[0];
                        llvm::errs() << "Parent node type: " << parent.getNodeKind().asStringRef() << "\n";
                        if(parent.getNodeKind().asStringRef()=="CompoundStmt")
                        {
                            const Stmt *stmt = currentNode.get<Stmt>();
                            SourceLocation beginLoc = stmt->getBeginLoc();
                            SourceLocation endLoc = stmt->getEndLoc();
                            if(beginLoc.isValid() && endLoc.isValid())
                            {
                                Rewrite.InsertText(beginLoc, "for (int i = 0; i < 10; ++i) {\n\t ", true, true);
                                
                                //Below code may result in } being placed before the ;
                                //Rewrite.InsertTextAfterToken(endLoc, ";\n\t}");
                                //Rewrite.InsertTextAfterToken(endLoc.getLocWithOffset(2), "\n\t}");

                                //Fix for the ; location problem is to get the exact location of the ; using Lexer. Below is the solution.
                                const SourceManager &SM = Context->getSourceManager();
                                SourceLocation semiLoc = Lexer::findNextToken(endLoc, SM, Context->getLangOpts())->getLocation();

                                if (semiLoc.isValid()) {
                                    // Adjust to be after the semicolon.
                                    SourceLocation AfterSemi = semiLoc.getLocWithOffset(1);
                                    Rewrite.InsertTextAfterToken(semiLoc, "\n\t}");
                                }
                            }
                            
                            break;
                        }
                        // Move to the parent for the next iteration
                        currentNode = parent;
                    }
                }
            }
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
