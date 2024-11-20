#ifndef SOURCE_PRINTER_H
#define SOURCE_PRINTER_H

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/CommandLine.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"

using namespace clang;

class SourcePrinterConsumer : public ASTConsumer {
public:
    explicit SourcePrinterConsumer(ASTContext &Context);

    void HandleTranslationUnit(ASTContext &Context) override;

private:
    const SourceManager &SM;
    void printSourceCode(const Decl *D);
};

class SourcePrinterAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override;
};

#endif // SOURCE_PRINTER_H
