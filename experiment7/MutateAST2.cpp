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
        if (!generateSourceCode && reachedMain && isInMainFile(FS)) {
            llvm::outs() << "====Found a for loop in the main file:====\n";
            //FS->dump(); // Print the for loop
            fs=FS;
        }
        
        return true; // Continue visiting other nodes.
    }

    bool VisitFunctionDecl(FunctionDecl *FD) {
        if(generateSourceCode)//for printing the AST
        {
            if (isInMainFile(FD) && FD->isThisDeclarationADefinition()) 
            {
                std::string FunctionSource;
                llvm::raw_string_ostream Stream(FunctionSource);

                // Generate source for the function
                FD->print(Stream);
                Stream.flush();
                llvm::outs()<<FunctionSource<<"\n\n";
            }
        }
        else
        {
            if (FD->getNameAsString() == "foo" && isInMainFile(FD)) {
                llvm::outs() << "====Found function foo in the main file:====\n";
                //FS->dump(); // Print the for loop
                stmt=FD->getBody();
            }
            else if(FD->isMain() && isInMainFile(FD))
            {   reachedMain=true;}
        }
        
        return true;
    }

    bool generateSourceCode=false;
private:
    bool reachedMain=false;
    ASTContext &Context;
    Stmt *stmt;//for created compound stmt C'
    ForStmt *fs;//For getting E


    
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
    
    Stmt* create_compound_stmt()
    {
        vector<string> args;
        args.push_back("-resource-dir");
        args.push_back("/usr/lib/clang/18");
        args.push_back("--cuda-gpu-arch=sm_86");
        string ForLoopCode = R"(
            #include<iostream>

            using namespace std;

            int add(int x,int y)
            {
                int z;
                z=x+y;
                return z;
            }

            int sub(int x,int y)
            {
                int z;
                z=x-y;
                return z;
            }
            int x,a;
            void foo() {//C'
                if(a>10)
                {
                    //C_loc
                }
                else
                {
                    x-=add(4,a);
                    x=x/4-sub(a,x);
                }
            }
        )";
        
        ForLoopAST = clang::tooling::buildASTFromCodeWithArgs(ForLoopCode, args);
        vi.TraverseDecl(ForLoopAST->getASTContext().getTranslationUnitDecl());
        return vi.return_stmt();
    }

public:
    explicit FunctionASTConsumer(ASTContext *mainContext) : mainASTContext(mainContext),vi(*mainContext) {}

    void HandleTranslationUnit(ASTContext &funcASTContext) override {

        llvm::outs()<<"\n\n====Creating compound (C') statement====\n";
        CompoundStmt *C_=dyn_cast<CompoundStmt>(create_compound_stmt());
        llvm::outs()<<"\n\n====Dumping Newly created CompoundStmt AST====\n";
        C_->dump();
        llvm::outs()<<"\n\n====Extracting IfStmt AST from C'====\n";
        llvm::SmallVector<Stmt *> Stmts(C_->body());
        IfStmt *C_if=dyn_cast<IfStmt>(Stmts[0]);
        C_if->dump();
        
        //get compound statement C
        /*
        for(int a=0;a<10;a++)//E
        {//C
            x+=add(4,7);
            x=x*4+sub(7,x);
        }
        */
        llvm::outs()<<"\n\n====Searching for CompoundStmt (E) AST====\n";
        vi.TraverseDecl(mainASTContext->getTranslationUnitDecl());
        ForStmt *E=vi.return_for_stmt();
        llvm::outs()<<"\n\n====Dumping ForStmt (E) AST====\n";
        E->dump();
        Stmt *C=E->getBody();
        llvm::outs()<<"\n\n====Dumping ForStmt (C) AST====\n";
        C->dump();

        llvm::outs()<<"\n\n====Adding C to C_if====\n";
        C_if->setThen(C);

        llvm::outs()<<"\n\n====Adding C' to E====\n";
        ForStmt *E_for=dyn_cast<ForStmt>(E);
        E_for->setBody(C_);

        E_for->dump();

        llvm::outs()<<"\n\n====Generating Source Code from AST====\n";
        vi.generateSourceCode=true;
        vi.TraverseDecl(mainASTContext->getTranslationUnitDecl());
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