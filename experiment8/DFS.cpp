#include "clang/Analysis/CallGraph.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include <iostream>
#include <unordered_set>
#include <stack>
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace clang::tooling;
using namespace std;

// Utility function for DFS
void DFS(CallGraphNode *Node, std::unordered_set<CallGraphNode *> &Visited) 
{
    if (!Node || Visited.count(Node)) return; // Skip null or already visited nodes

    Visited.insert(Node); // Mark the node as visited

    const auto *Decl = Node->getDecl();
    if (const auto *FD = dyn_cast_or_null<FunctionDecl>(Decl)) {
        std::cout << "Visiting: " << FD->getNameAsString() << "\n";
    }

    // Recursively visit all callees
    for (CallGraphNode *Callee : *Node) 
    {   DFS(Callee, Visited);}
}

// Iterative DFS (optional alternative to recursion)
void DFSIterative(CallGraphNode *StartNode) {
    if (!StartNode) return;

    std::unordered_set<CallGraphNode *> Visited;
    std::stack<CallGraphNode *> Stack;

    Stack.push(StartNode);

    while (!Stack.empty()) {
        CallGraphNode *Node = Stack.top();
        Stack.pop();

        if (Visited.count(Node)) continue;

        Visited.insert(Node);
        const auto *Decl = Node->getDecl();
        if (const auto *FD = dyn_cast_or_null<FunctionDecl>(Decl)) {
            std::cout << "Visiting: " << FD->getNameAsString() << "\n";
        }

        for (CallGraphNode *Callee : *Node) {
            if (!Visited.count(Callee)) {
                Stack.push(Callee);
            }
        }
    }
}

class CallGraphConsumer : public ASTConsumer {
public:
    explicit CallGraphConsumer(ASTContext &Context) : Context(Context) {}

    void HandleTranslationUnit(ASTContext &Context) override {
        CallGraph CG;
        CG.addToCallGraph(Context.getTranslationUnitDecl());

        // Start DFS from the 'main' function
        for (auto it = CG.begin(); it != CG.end(); ++it) {
            const Decl *D = it->first;
            if (!D) continue;

            if (const auto *FD = dyn_cast<FunctionDecl>(D)) {
                if (FD->getNameAsString() == "main") {
                    std::cout << "Starting DFS from 'main':\n";
                    std::unordered_set<CallGraphNode *> Visited;
                    DFS(it->second.get(), Visited);
                    break;
                }
            }
        }
    }

private:
    ASTContext &Context;
};

class CallGraphAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef File) override {
        return std::make_unique<CallGraphConsumer>(CI.getASTContext());
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
    Tool.run(newFrontendActionFactory<CallGraphAction>().get());

    return 0;
}