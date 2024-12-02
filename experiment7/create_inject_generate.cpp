#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"

#include "clang/Sema/Sema.h"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace clang::tooling;
using namespace std;

string getFileContent(const string &filename) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> bufferOrError =
        llvm::MemoryBuffer::getFile(filename);

    if (error_code ec = bufferOrError.getError()) {
        llvm::errs() << "Error reading file " << filename << ": " << ec.message() << "\n";
        return "";
    }

    return bufferOrError.get()->getBuffer().str();
}

unique_ptr<ASTUnit> loadASTFromCode(const string &code, const vector<string> &args) {
    return tooling::buildASTFromCodeWithArgs(code, args);
}



/// Visitor to traverse the AST and find `ForStmt` nodes.
class ForLoopVisitor : public RecursiveASTVisitor<ForLoopVisitor> {
public:
    explicit ForLoopVisitor(ASTContext &Context) : Context(Context) {}

    bool VisitForStmt(ForStmt *FS) {
        llvm::outs() << "Found a for loop at line "<< Context.getSourceManager().getSpellingLineNumber(FS->getForLoc()) << ":\n";
        //FS->dump();
        fs=FS;
        return true; // Continue visiting other nodes.
    }

    ForStmt* return_for_stmt()
    {   return fs;}

private:
    ASTContext &Context;
    ForStmt *fs;
};

class FunctionASTConsumer : public ASTConsumer {
    ASTContext *mainASTContext;
    ForLoopVisitor Visitor;
public:
    explicit FunctionASTConsumer(ASTContext *mainContext) : mainASTContext(mainContext),Visitor(*mainContext) {}

    void HandleTranslationUnit(ASTContext &funcASTContext) override {
        TranslationUnitDecl *mainTUDecl = mainASTContext->getTranslationUnitDecl();
        Visitor.TraverseDecl(mainTUDecl);
        Visitor.return_for_stmt()->dump();
    }
};


class ASTAction : public ASTFrontendAction {
    ASTContext *mainASTContext;

public:
    explicit ASTAction(ASTContext *mainContext) : mainASTContext(mainContext) {}
    
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, llvm::StringRef file) override {
        return std::make_unique<FunctionASTConsumer>(mainASTContext);
    }
};

int main(int argc, const char **argv)
{
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
    string mainFile = files[0];
    //ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    //Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("--cuda-gpu-arch=sm_86", ArgumentInsertPosition::END));  // Specify CUDA architecture
    //Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-resource-dir", ArgumentInsertPosition::END));
    //Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("/usr/lib/clang/18", ArgumentInsertPosition::END));

    string mainCode = getFileContent(mainFile);
    if (mainCode.empty()) {
        llvm::errs() << "Failed to read main file.\n";
        return 1;
    }

    //generate main file ast
    vector<string> args;
    args.push_back("-resource-dir");
    args.push_back("/usr/lib/clang/18");
    args.push_back("--cuda-gpu-arch=sm_86");

    auto mainAST = loadASTFromCode(mainCode, args);
    if (!mainAST) {
        llvm::errs() << "Failed to parse main file.\n";
        return 1;
    }

    //create for loop ast node from string

    std::string ForLoopCode = R"(
        void foo() {
        for (int i = 0; i < 10; ++i) {
            // Loop body
        }
        }
    )";
    auto ForLoopAST = clang::tooling::buildASTFromCodeWithArgs(ForLoopCode, args);
    if (ForLoopAST) {
        ForLoopAST->getASTContext().getTranslationUnitDecl()->dump();
    } else {
        llvm::errs() << "Failed to parse the input code.\n";
    }

    if (!tooling::runToolOnCodeWithArgs(
            std::make_unique<ASTAction>(&ForLoopAST->getASTContext()), ForLoopCode, args)) {
        llvm::errs() << "Failed to parse function file with main file context.\n";
        return 1;
    }

    return 0;
}