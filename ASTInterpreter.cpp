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
			std::cout << "Processing BinaryOperator" << std::endl;
		#endif
		mEnv->binop(bop);
	}

	virtual void VisitUnaryOperator (clang::UnaryOperator * uop) {
		VisitStmt(uop);
		#ifdef _DEBUG
			std::cout << "Processing UnaryOperator" << std::endl;
		#endif
		mEnv->unop(uop);
	}

	virtual void VisitDeclRefExpr(clang::DeclRefExpr * expr) {
		VisitStmt(expr);
		#ifdef _DEBUG
			std::cout << "Processing DeclRefExpr" << std::endl;
		#endif
		mEnv->declref(expr);
	}

	virtual void VisitCastExpr(clang::CastExpr * expr) {
		VisitStmt(expr);
		#ifdef _DEBUG
			std::cout << "Processing CastExpr" << std::endl;
		#endif
		mEnv->cast(expr);
	}

	virtual void VisitCallExpr(clang::CallExpr * call) {
		VisitStmt(call);
		#ifdef _DEBUG
			std::cout << "Processing CallExpr" << std::endl;
		#endif
		if (mEnv->beforeCall(call)) {
			VisitStmt(call->getDirectCallee()->getBody());
			mEnv->afterCall(call);
		}
	}

	virtual void VisitArraySubscriptExpr(clang::ArraySubscriptExpr * array) {
		VisitStmt(array);
		#ifdef _DEBUG
			std::cout << "Processing ArraySubscriptExpr" << std::endl;
		#endif
		mEnv->array(array);
	}

	virtual void VisitDeclStmt(clang::DeclStmt * declstmt) {
		VisitStmt(declstmt);
		#ifdef _DEBUG
			std::cout << "Processing DeclStmt" << std::endl;
		#endif
		mEnv->decl(declstmt, mContext);
	}

	virtual void VisitIntegerLiteral(clang::IntegerLiteral * literal) {
		#ifdef _DEBUG
			std::cout << "Processing IntegerLiteral" << std::endl;
		#endif
		mEnv->literal(literal, mContext);
	}

	virtual void VisitIfStmt(clang::IfStmt * ifstmt) {
		#ifdef _DEBUG
			std::cout << "Processing IfStmt" << std::endl;
		#endif
		clang::Expr * cond = ifstmt->getCond();
		clang::Stmt * then = ifstmt->getThen();
		clang::Stmt * els = ifstmt->getElse();
		//VisitStmt()只访问所有子stmt，而Visit()同时访问stmt本身，但要求stmt不为空指针
		Visit(cond);
		if (mEnv->cond(cond)) {
			#ifdef _DEBUG
				std::cout << " Into then" << std::endl;
			#endif
			Visit(then);
		} else if (els) {
			#ifdef _DEBUG
				std::cout << " Into else" << std::endl;
			#endif
			Visit(els);
		}
	}

	virtual void VisitWhileStmt(clang::WhileStmt * whilestmt) {
		#ifdef _DEBUG
			int _i = 1;
		#endif
		clang::Expr * cond = whilestmt->getCond();
		clang::Stmt * body = whilestmt->getBody();
		while (true) {
			Visit(cond);
			if (!mEnv->cond(cond)) break;
			#ifdef _DEBUG
				std::cout << "Processing WhileStmt " << _i++ << std::endl;
			#endif
			//getbody()返回类型为CompundStmt
			VisitStmt(body);
		}
	}

	virtual void VisitForStmt(clang::ForStmt * forstmt) {
		#ifdef _DEBUG
			int _i = 1;
		#endif
		clang::Stmt * init = forstmt->getInit();
		clang::Expr * cond = forstmt->getCond();
		clang::Expr * inc = forstmt->getInc();
		clang::Stmt * body = forstmt->getBody();
		if (init) Visit(init);
		while (true) {
			Visit(cond);
			if (!mEnv->cond(cond)) break;
			#ifdef _DEBUG
				std::cout << "Processing ForStmt " << _i++ << std::endl;
			#endif
			VisitStmt(body);
			if (inc) Visit(inc);
		}
	}

	virtual void VisitReturnStmt(clang::ReturnStmt * restmt) {
		VisitStmt(restmt);
		#ifdef _DEBUG
			std::cout << "Processing ReturnStmt" << std::endl;
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
