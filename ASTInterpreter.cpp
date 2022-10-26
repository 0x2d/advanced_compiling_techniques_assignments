//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#include "Environment.h"

class InterpreterVisitor : public clang::EvaluatedExprVisitor<InterpreterVisitor> {
public:
	explicit InterpreterVisitor(const clang::ASTContext &context, Environment * env) 
		: EvaluatedExprVisitor(context), mContext(context), mEnv(env) {}
	virtual ~InterpreterVisitor() {};

	virtual void VisitBinaryOperator (clang::BinaryOperator * bop) {
		VisitStmt(bop);
		#ifdef _DEBUG
			std::cout << "Processing BinaryOperator " << bop << std::endl;
		#endif
		mEnv->binop(bop);
	}

	virtual void VisitUnaryOperator (clang::UnaryOperator * uop) {
		VisitStmt(uop);
		#ifdef _DEBUG
			std::cout << "Processing UnaryOperator " << uop << std::endl;
		#endif
		mEnv->unop(uop);
	}

	virtual void VisitDeclRefExpr(clang::DeclRefExpr * expr) {
		VisitStmt(expr);
		#ifdef _DEBUG
			std::cout << "Processing DeclRefExpr " << expr << std::endl;
		#endif
		mEnv->declref(expr);
	}

	virtual void VisitCastExpr(clang::CastExpr * expr) {
		VisitStmt(expr);
		#ifdef _DEBUG
			std::cout << "Processing CastExpr " << expr << std::endl;
		#endif
		mEnv->cast(expr);
	}

	virtual void VisitCallExpr(clang::CallExpr * call) {
		VisitStmt(call);
		#ifdef _DEBUG
			std::cout << "Processing CallExpr " << call << std::endl;
		#endif
		if (mEnv->beforeCall(call)) {
			VisitStmt(call->getDirectCallee()->getBody());
			mEnv->afterCall(call);
		}
	}

	virtual void VisitDeclStmt(clang::DeclStmt * declstmt) {
		VisitStmt(declstmt);
		#ifdef _DEBUG
			std::cout << "Processing DeclStmt " << declstmt << std::endl;
		#endif
		mEnv->decl(declstmt, mContext);
	}

	virtual void VisitIntegerLiteral(clang::IntegerLiteral * literal) {
		#ifdef _DEBUG
			std::cout << "Processing IntegerLiteral " << literal << std::endl;
		#endif
		mEnv->literal(literal, mContext);
	}

	virtual void VisitIfStmt(clang::IfStmt * ifstmt) {
		#ifdef _DEBUG
			std::cout << "Processing IfStmt " << ifstmt << std::endl;
		#endif
		clang::Expr * cond = ifstmt->getCond();
		clang::Stmt * then = ifstmt->getThen();
		clang::Stmt * els = ifstmt->getElse();
		//VisitStmt方法只访问所有子stmt，而Visit方法同时访问stmt本身
		Visit(cond);
		if (mEnv->cond(cond)) {
			Visit(then);
		} else {
			Visit(els);
		}
	}

	virtual void VisitWhileStmt(clang::WhileStmt * whilestmt) {
		#ifdef _DEBUG
			std::cout << "Processing WhileStmt " << whilestmt << std::endl;
		#endif
		clang::Expr * cond = whilestmt->getCond();
		clang::Stmt * body = whilestmt->getBody();
		while (true) {
			Visit(cond);
			if (!mEnv->cond(cond)) break;
			//getbody()返回类型为CompundStmt
			VisitStmt(body);
		}
	}

	virtual void VisitForStmt(clang::ForStmt * forstmt) {
		#ifdef _DEBUG
			std::cout << "Processing ForStmt " << forstmt << std::endl;
		#endif
		clang::Stmt * init = forstmt->getInit();
		clang::Expr * cond = forstmt->getCond();
		clang::Expr * inc = forstmt->getInc();
		clang::Stmt * body = forstmt->getBody();
		Visit(init);
		while (true) {
			Visit(cond);
			if (!mEnv->cond(cond)) break;
			VisitStmt(body);
			Visit(inc);
		}
	}

	virtual void VisitReturnStmt(clang::ReturnStmt * restmt) {
		VisitStmt(restmt);
		#ifdef _DEBUG
			std::cout << "Processing ReturnStmt " << restmt << std::endl;
		#endif
		mEnv->returnStmt(restmt);
	}

private:
	const clang::ASTContext& mContext;
	Environment * mEnv;
};

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
