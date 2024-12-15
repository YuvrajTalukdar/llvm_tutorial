#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

// Define a callback class to handle the matches
class IfStatementMatcher : public MatchFinder::MatchCallback 
{
public:
    const CompoundStmt *Compound;
    void run(const MatchFinder::MatchResult &Result) override 
    {
        const IfStmt *If = Result.Nodes.getNodeAs<IfStmt>("ifStmt");
        if(If) 
        {
            // Check if the 'if' statement has a compound statement inside it
            Compound = dyn_cast<CompoundStmt>(If->getThen());
            if(Compound) 
            {
                llvm::outs() << "Found a CompoundStmt inside the if statement.\n";
                // You can iterate through the statements in the CompoundStmt here
                Compound->dump();
                for(auto stmt : Compound->body()) 
                {
                    // Optionally, print the statement or do something else with it
                    stmt->dump();
                }
            }
        }
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
    if (files.size() != 1) {
        llvm::errs() << "Expected exactly 1 files: main file and function file.\n";
        return 1;
    }
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("--cuda-gpu-arch=sm_86", ArgumentInsertPosition::END));  // Specify CUDA architecture
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-resource-dir", ArgumentInsertPosition::END));
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("/usr/lib/clang/18", ArgumentInsertPosition::END));
    
    // Create the matcher for the 'if' statement with the specific condition
    /*StatementMatcher IfStmtMatcher = ifStmt(
        hasCondition(binaryOperator(
            hasOperatorName("<"),
            hasLHS(ignoringImpCasts(declRefExpr(to(varDecl(hasName("id_current")))))),
            hasRHS(ignoringImpCasts(declRefExpr(to(varDecl(hasName("max_id_original"))))))
        ))
    ).bind("ifStmt");*/

    StatementMatcher IfStmtMatcher = ifStmt(
        hasCondition(binaryOperator(
            hasOperatorName("<"),
            hasLHS(ignoringImpCasts(declRefExpr(to(varDecl(hasName("id_current")))))),
            hasRHS(ignoringImpCasts(declRefExpr(to(varDecl(hasName("max_id_original"))))))
        )),
        hasAncestor(functionDecl(hasName("foo123")))
    ).bind("ifStmt");

    // Set up the matcher and the callback
    MatchFinder Finder;
    IfStatementMatcher Matcher;
    Finder.addMatcher(IfStmtMatcher, &Matcher);

    // Run the tool
    return Tool.run(tooling::newFrontendActionFactory(&Finder).get());
}
