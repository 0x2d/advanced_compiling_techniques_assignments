//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#include <cstdio>
#include <string>
#include <iostream>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

class InterpreterVisitor;

class StackFrame {
	/// StackFrame maps Variable Declaration to Value
	/// Which are either integer or addresses (also represented using an Integer value)
	std::map<clang::Decl*, int> mVars;
	std::map<clang::Stmt*, int> mExprs;
	/// The current stmt
	clang::Stmt * mPC;
	int returnVal;

public:
	StackFrame() : mVars(), mExprs(), mPC() {}

	void bindDecl(clang::Decl* decl, int val) {
		mVars[decl] = val;
	}

	bool findDeclVal(clang::Decl * decl) {
		return mVars.find(decl) != mVars.end();
	}

	int getDeclVal(clang::Decl * decl) {
		assert (mVars.find(decl) != mVars.end());
		return mVars.find(decl)->second;
	}

	void bindStmt(clang::Stmt * stmt, int val) {
		mExprs[stmt] = val;
	}

	bool findStmtVal(clang::Stmt * stmt) {
		return mExprs.find(stmt) != mExprs.end();
	}

	int getStmtVal(clang::Stmt * stmt) {
		assert (mExprs.find(stmt) != mExprs.end());
		return mExprs[stmt];
	}

	void setPC(clang::Stmt * stmt) {
		mPC = stmt;
	}

	clang::Stmt * getPC() {
		return mPC;
	}

	void setReturn(int r) {
		returnVal = r;
	}

	int getReturn() {
		return returnVal;
	}
};

/// Heap maps address to a value
class Heap {
	std::map<clang::Decl*, int> mVars;
	std::map<clang::Stmt*, int> mExprs;

public:
	Heap() : mVars() {}

	void bindDecl(clang::Decl* decl, int val) {
		mVars[decl] = val;
	}

	int getDeclVal(clang::Decl * decl) {
		assert (mVars.find(decl) != mVars.end());
		return mVars.find(decl)->second;
	}

	void bindStmt(clang::Stmt * stmt, int val) {
		mExprs[stmt] = val;
	}

	int getStmtVal(clang::Stmt * stmt) {
		assert (mExprs.find(stmt) != mExprs.end());
		return mExprs[stmt];
	}

	int Malloc(int size) ;
	void Free (int addr) ;
	void Update(int addr, int val) ;
	int get(int addr);
};

class Environment {
	std::vector<StackFrame> mStack;
	Heap mHeap;

	clang::FunctionDecl * mFree;				/// Declartions to the built-in functions
	clang::FunctionDecl * mMalloc;
	clang::FunctionDecl * mInput;
	clang::FunctionDecl * mOutput;
	clang::FunctionDecl * mEntry;
	std::vector<clang::FunctionDecl*> mFuncs;

public:
	/// Get the declartions to the built-in functions
	Environment() : mStack(), mHeap(), mFree(NULL), mMalloc(NULL), mInput(NULL), mOutput(NULL), mEntry(NULL), mFuncs() {}

	/// Initialize the Environment
	void init(clang::TranslationUnitDecl * unit, const clang::ASTContext& context);

	clang::FunctionDecl * getEntry() { return mEntry; }

	void binop(clang::BinaryOperator * bop, const clang::ASTContext& context);
	void literal(clang::IntegerLiteral * literal, const clang::ASTContext& context);
	void decl(clang::DeclStmt * declstmt);
	void declref(clang::DeclRefExpr * declref);
	void cast(clang::CastExpr * castexpr);
	void call(clang::CallExpr * callexpr, InterpreterVisitor * mVisitor);
	void ifstmt(clang::IfStmt * ifstmt);
};

class InterpreterVisitor : public clang::EvaluatedExprVisitor<InterpreterVisitor> {
public:
	explicit InterpreterVisitor(const clang::ASTContext &context, Environment * env) 
		: EvaluatedExprVisitor(context), mContext(context), mEnv(env) {}
	virtual ~InterpreterVisitor() {};

	virtual void VisitBinaryOperator (clang::BinaryOperator * bop) {
		VisitStmt(bop);
		mEnv->binop(bop, mContext);
	}

	virtual void VisitDeclRefExpr(clang::DeclRefExpr * expr) {
		VisitStmt(expr);
		mEnv->declref(expr);
	}

	virtual void VisitCastExpr(clang::CastExpr * expr) {
		VisitStmt(expr);
		mEnv->cast(expr);
	}

	virtual void VisitCallExpr(clang::CallExpr * call) {
		VisitStmt(call);
		mEnv->call(call, this);
	}

	virtual void VisitDeclStmt(clang::DeclStmt * declstmt) {
		mEnv->decl(declstmt);
	}

	virtual void VisitIntegerLiteral(clang::IntegerLiteral * literal) {
		mEnv->literal(literal, mContext);
	}

	virtual void VisitIfStmt(clang::IfStmt * ifstmt) {
		VisitStmt(ifstmt);
		mEnv->ifstmt(ifstmt);
	}

private:
	const clang::ASTContext& mContext;
	Environment * mEnv;
};
