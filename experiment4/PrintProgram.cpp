
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "PrintProgram.h"

using namespace clang::tooling;

SourcePrinterConsumer::SourcePrinterConsumer(ASTContext &Context)
    : SM(Context.getSourceManager()) {}

void SourcePrinterConsumer::HandleTranslationUnit(ASTContext &Context) {
    const TranslationUnitDecl *TU = Context.getTranslationUnitDecl();
    for (const Decl *D : TU->decls()) {
        if (Context.getSourceManager().isInMainFile(D->getLocation())) {
            printSourceCode(D);
        }
    }
}

void SourcePrinterConsumer::printSourceCode(const Decl *D) {
    SourceRange SR = D->getSourceRange();
    bool Invalid = false;
    StringRef SourceText = Lexer::getSourceText(CharSourceRange::getTokenRange(SR), SM, LangOptions(), &Invalid);
    if (!Invalid && !SourceText.empty()) {
        if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(D)) {
            llvm::outs() << SourceText << "\n";
        } else if (const VarDecl *VD = dyn_cast<VarDecl>(D)) {
            llvm::outs() << SourceText << ";\n";
        }
    }
}

std::unique_ptr<ASTConsumer> SourcePrinterAction::CreateASTConsumer(CompilerInstance &CI, StringRef InFile) {
    return std::make_unique<SourcePrinterConsumer>(CI.getASTContext());
}


