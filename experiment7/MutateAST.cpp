#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ParentMapContext.h"

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
class Visitor : public RecursiveASTVisitor<Visitor> {
public:
    explicit Visitor(ASTContext &Context) : Context(Context) {}

    Stmt* return_stmt()
    {   return stmt;}

    ForStmt* return_for_stmt()
    {   return fs;}

    bool VisitForStmt(ForStmt *FS) {
        if (!generateSourceCode && isInMainFile(FS)) {
            llvm::outs() << "====Found a for loop in the main file:====\n";
            //FS->dump(); // Print the for loop
            fs=FS;
        }
        
        return true; // Continue visiting other nodes.
    }

    bool VisitFunctionDecl(FunctionDecl *FD) {
        if (generateSourceCode && isInMainFile(FD) && FD->isThisDeclarationADefinition()) {
            std::string FunctionSource;
            llvm::raw_string_ostream Stream(FunctionSource);

            // Generate source for the function
            FD->print(Stream);
            Stream.flush();
            llvm::outs()<<FunctionSource<<"\n\n";
        }
        return true;
    }

    bool VisitStmt(Stmt *S) 
    {
        if(generateSourceCode)
        {
            //llvm::outs()<<stmtToString(S);
        }
        else
        {
            if (auto *CallExprNode = dyn_cast<CallExpr>(S)) 
            {
                if(const FunctionDecl *FD = CallExprNode->getDirectCallee())
                {
                    if (FD->getNameAsString() == "add") 
                    {
                        //llvm::outs()<<"\nCheck4 "<<a<<"\n";
                        llvm::outs()<<"\n\n====add function call found====\n";
                        stmt=S;
                    }
                }
            }
        }
        
        return true;
    }

    bool generateSourceCode=false;
private:
    ASTContext &Context;
    Stmt *stmt;
    ForStmt *fs;
    
    bool isInMainFile(const Stmt *S) {
        const SourceManager &SM = Context.getSourceManager();
        SourceLocation Loc = S->getBeginLoc();
        // Ensure the location is valid and comes from the main file
        return Loc.isValid() && SM.isInMainFile(Loc);
    }

    bool isInMainFile(const FunctionDecl *S) {
        const SourceManager &SM = Context.getSourceManager();
        SourceLocation Loc = S->getBeginLoc();
        // Ensure the location is valid and comes from the main file
        return Loc.isValid() && SM.isInMainFile(Loc);
    }

    string stmtToString(Stmt *stmt)
    {
        clang::LangOptions lo;
        string out_str;
        llvm::raw_string_ostream outstream(out_str);
        stmt->printPretty(outstream, NULL, PrintingPolicy(lo));
        return out_str;
    }
};

class FunctionASTConsumer : public ASTConsumer {
private:
    ASTContext *mainASTContext;
    unique_ptr<clang::ASTUnit, std::default_delete<clang::ASTUnit>> ForLoopAST;
    Visitor vi;
    const Stmt* get_parent_node(Stmt *stmt)
    {
        DynTypedNode currentNode = mainASTContext->getParents(*stmt)[0];
        while (true) 
        {
            const DynTypedNodeList &parents = mainASTContext->getParents(currentNode);
            if (parents.empty()) {
                llvm::errs() << "No more parents.\n";
                return NULL;
            }

            // Process each parent (usually there's just one)
            const auto &parent = parents[0];
            llvm::outs() << "====Found Parent node type: " << parent.getNodeKind().asStringRef() << "====\n";
            if(parent.getNodeKind().asStringRef()=="CompoundStmt")
            {   return currentNode.get<Stmt>();}
            // Move to the parent for the next iteration
            currentNode = parent;
        }
    }
    
    ForStmt* create_for_stmt()
    {
        vector<string> args;
        args.push_back("-resource-dir");
        args.push_back("/usr/lib/clang/18");
        args.push_back("--cuda-gpu-arch=sm_86");
        string ForLoopCode = R"(
            void foo() {
            for (int i = 0; i < 10; ++i) {
                // Loop body
            }
            }
        )";
        
        ForLoopAST = clang::tooling::buildASTFromCodeWithArgs(ForLoopCode, args);
        vi.TraverseDecl(ForLoopAST->getASTContext().getTranslationUnitDecl());
        return vi.return_for_stmt();
    }

public:
    explicit FunctionASTConsumer(ASTContext *mainContext) : mainASTContext(mainContext),vi(*mainContext) {}

    void HandleTranslationUnit(ASTContext &funcASTContext) override {
        TranslationUnitDecl *mainTUDecl = mainASTContext->getTranslationUnitDecl();
        //vi.TraverseDecl(mainTUDecl);
        //Visitor.return_for_stmt()->dump();

        Decl *decl=NULL;
        Stmt *Body=NULL;
        FunctionDecl *fd=NULL;
        for(Decl *d : mainTUDecl->decls())
        {
            if (auto *FD = dyn_cast<FunctionDecl>(d)) 
            {
                if (FD->getNameAsString() == "main") 
                {
                    if (FD->isThisDeclarationADefinition()) 
                    {
                        decl=d;
                        Body=FD->getBody();
                        fd=FD;
                        llvm::outs() << "\n\n====Found definition of function 'main'====\n";
                        FD->dump(); // Dump the AST node for verification (optional)
                        break;
                    }
                }
            }
        }
        
        if (Body!=NULL) 
        {
            llvm::outs()<<"\n\n====Creating for statement====\n";
            ForStmt *fs=create_for_stmt();
            llvm::outs()<<"\n\n====Dumping Newly created ForStmt AST====\n";
            fs->dump();
            //get parent statement for function call add
            vi.TraverseDecl(mainASTContext->getTranslationUnitDecl());
            Stmt *addStmt=vi.return_stmt();
            llvm::outs()<<"\n\n====searching for add functions its root compoundStmt====\n";
            const Stmt* parentCompountStmt=get_parent_node(addStmt);
            
            if (auto *Compound = dyn_cast<CompoundStmt>(Body)) 
            {
                // Retrieve existing statements
                llvm::SmallVector<Stmt *> Stmts(Compound->body());
                SmallVector<Stmt*> mutated_stmt; 

                for(int a=0;a<Stmts.size();a++)
                {
                    if(parentCompountStmt==Stmts[a])
                    {
                        llvm::outs()<<"\n\n====Match found!! Adding new body to generated ForStmt====\n";
                        //constructing mutated statement.
                        Stmt *MutableStmt = const_cast<Stmt*>(parentCompountStmt);
                        fs->setBody(MutableStmt);
                        mutated_stmt.push_back(fs);
                    }
                    else
                    {   mutated_stmt.push_back(Stmts[a]);}
                }
                llvm::outs()<<"\n\n====Creating Mutated CompoundStmt====\n";
                clang::FPOptionsOverride DefaultFPOptions = clang::FPOptionsOverride();
                CompoundStmt *NewBody = CompoundStmt::Create(
                    *mainASTContext,                         // ASTContext
                    mutated_stmt,                           // List of statements
                    DefaultFPOptions,      // Default FPOptions
                    Compound->getLBracLoc(),         // Opening brace location
                    Compound->getRBracLoc()          // Closing brace location
                );
                llvm::outs()<<"\n\n====Setting NewBody====\n";
                //NewBody->dump();
                fd->setBody(NewBody);
                llvm::outs()<<"\n\n====Generating Source Code from AST====\n";
                vi.generateSourceCode=true;
                vi.TraverseDecl(mainASTContext->getTranslationUnitDecl());
                //mainTUDecl->dump();
                
            }
        }
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

    if (!tooling::runToolOnCodeWithArgs(
            std::make_unique<ASTAction>(&mainAST->getASTContext()), mainCode, args)) {
        llvm::errs() << "Failed to parse function file with main file context.\n";
        return 1;
    }

    return 0;
}