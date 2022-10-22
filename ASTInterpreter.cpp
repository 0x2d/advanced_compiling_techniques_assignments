//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#include "Environment.h"

class InterpreterConsumer : public clang::ASTConsumer {
public:
	explicit InterpreterConsumer(const clang::ASTContext& context) : mEnv(), mVisitor(context, &mEnv) {}
	virtual ~InterpreterConsumer() {}

	virtual void HandleTranslationUnit(clang::ASTContext &Context) {
		clang::TranslationUnitDecl * decl = Context.getTranslationUnitDecl();
		mEnv.init(decl, Context);
		clang::FunctionDecl * entry = mEnv.getEntry();
		mVisitor.VisitStmt(entry->getBody());
	}

private:
	Environment mEnv;
	InterpreterVisitor mVisitor;
};

class InterpreterClassAction : public clang::ASTFrontendAction {
public: 
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::unique_ptr<clang::ASTConsumer>(new InterpreterConsumer(Compiler.getASTContext()));
  }
};

int main (int argc, char ** argv) {
   if (argc > 1) {
       clang::tooling::runToolOnCode(std::unique_ptr<clang::FrontendAction>(new InterpreterClassAction), argv[1]);
   }
}
