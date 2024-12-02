#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::tooling;

// Function to read file contents into a string
std::string getFileContent(const std::string &filename) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> bufferOrError =
        llvm::MemoryBuffer::getFile(filename);

    if (std::error_code ec = bufferOrError.getError()) {
        llvm::errs() << "Error reading file " << filename << ": " << ec.message() << "\n";
        return "";
    }

    return bufferOrError.get()->getBuffer().str();
}

std::unique_ptr<ASTUnit> loadASTFromCode(const std::string &code, const std::vector<std::string> &args) {
    return tooling::buildASTFromCodeWithArgs(code, args);
}

void printAST(ASTContext &context, const std::string &title) {
    llvm::outs() << "=== " << title << " ===\n";
    context.getTranslationUnitDecl()->dump();
}


class SourceGenerator : public RecursiveASTVisitor<SourceGenerator> {
public:
    SourceGenerator(ASTContext &context, Rewriter &rewriter)
        : Context(context), TheRewriter(rewriter) {}

    bool VisitFunctionDecl(FunctionDecl *FD) {
        if (FD->isThisDeclarationADefinition()) {
            std::string FunctionSource;
            llvm::raw_string_ostream Stream(FunctionSource);

            // Generate source for the function
            FD->print(Stream);
            Stream.flush();
            
            llvm::outs()<<FunctionSource<<"\n\n";
            
            // Insert the function definition into the rewritten output
            //TheRewriter.InsertText(FD->getBeginLoc(), FunctionSource + "\n\n", true, true);
        }
        return true;
    }

private:
    ASTContext &Context;
    Rewriter &TheRewriter;
};

void GenerateSource(ASTContext &funcASTContext) {
    Rewriter TheRewriter;
    TheRewriter.setSourceMgr(funcASTContext.getSourceManager(), funcASTContext.getLangOpts());

    // Traverse the updated AST and generate source
    SourceGenerator Generator(funcASTContext, TheRewriter);
    //llvm::outs()<<"check3\n\n";
    Generator.TraverseDecl(funcASTContext.getTranslationUnitDecl());
    

    /*llvm::outs()<<"check4\n\n";
    const RewriteBuffer *RewriteBuf = TheRewriter.getRewriteBufferFor(funcASTContext.getSourceManager().getMainFileID());
    
    llvm::outs()<<"check5\n\n";
    if (RewriteBuf) {
        llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end()) << "\n";
        //llvm::outs() << RewriteBuf->toString() << "\n";
    } else {
        llvm::outs() << "No modifications made to the source.\n";
    }*/
}


class FunctionASTConsumer : public ASTConsumer {
    ASTContext *mainASTContext;

public:
    explicit FunctionASTConsumer(ASTContext *mainContext) : mainASTContext(mainContext) {}

    void HandleTranslationUnit(ASTContext &funcASTContext) override {
        llvm::outs() << "=== Parsing Function File with Main File Context ===\n";

        TranslationUnitDecl *mainTUDecl = mainASTContext->getTranslationUnitDecl();
        TranslationUnitDecl *funcTUDecl = funcASTContext.getTranslationUnitDecl();

        // Inject relevant declarations from the main AST into the function AST
        llvm::outs() << "Injected declarations from main AST:\n";
        for (Decl *decl : mainTUDecl->decls()) {
            if (isa<FunctionDecl>(decl)) { // Add only function declarations
                funcTUDecl->addDecl(decl);
            }
        }

        //funcTUDecl->dump();
        // Continue processing the function AST
        std::string astDump;
        llvm::raw_string_ostream stream(astDump);
        funcASTContext.getTranslationUnitDecl()->dump(stream);
        stream.flush();
        // Print the AST dump to std::cout
        //llvm::outs() << astDump <<"\n";
        //std::cout << astDump <<"\n";


        GenerateSource(funcASTContext);
        //GenerateSource(*mainASTContext);
    }
};

class FunctionASTAction : public ASTFrontendAction {
    ASTContext *mainASTContext;

public:
    explicit FunctionASTAction(ASTContext *mainContext) : mainASTContext(mainContext) {}

    // Provide a custom ASTConsumer
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, llvm::StringRef file) override {
        return std::make_unique<FunctionASTConsumer>(mainASTContext);
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
    if (files.size() != 2) {
        llvm::errs() << "Expected exactly two files: main file and function file.\n";
        return 1;
    }

    const std::string mainFile = files[0];
    const std::string funcFile = files[1];
    std::vector<std::string> args;// = OptionsParser.getCompilations().getCompileCommands(mainFile)[0].CommandLine;
    
    // Read the contents of the main file
    std::string mainCode = getFileContent(mainFile);
    if (mainCode.empty()) {
        llvm::errs() << "Failed to read main file.\n";
        return 1;
    }

    std::string arg1="-resource-dir";
    std::string arg2="/usr/lib/clang/18";
    args.push_back(arg1);
    args.push_back(arg2);

    // Load AST for the main file
    auto mainAST = loadASTFromCode(mainCode, args);
    if (!mainAST) {
        llvm::errs() << "Failed to parse main file.\n";
        return 1;
    }

    // Load the main file's AST
    auto mainASTUnit = tooling::buildASTFromCodeWithArgs(getFileContent(mainFile), args);
    if (!mainASTUnit) {
        llvm::errs() << "Failed to parse main file.\n";
        return 1;
    }

    if (!tooling::runToolOnCodeWithArgs(
            std::make_unique<FunctionASTAction>(&mainASTUnit->getASTContext()), getFileContent(funcFile), args)) {
        llvm::errs() << "Failed to parse function file with main file context.\n";
        return 1;
    }

    return 0;
}
