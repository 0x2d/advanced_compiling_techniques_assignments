//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <stdexcept>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

//调试开关
// #define _DEBUG

class InterpreterVisitor;

class StackFrame {
	/// StackFrame maps Variable Declaration to Value
	/// Which are either integer or addresses (also represented using an Integer value)
	std::map<clang::Decl*, int64_t> mVars;
	std::map<clang::Stmt*, int64_t> mExprs;
	// 保存地址
	std::map<clang::Stmt*, int64_t> mPointers;
	/// The current stmt
	clang::Stmt * mPC;
	int returnVal;

public:
	StackFrame() : mVars(), mExprs(), mPC() {}

	//Vars
	void bindDecl(clang::Decl* decl, int64_t val) {
		mVars[decl] = val;
	}

	bool findDeclVal(clang::Decl * decl) {
		return mVars.find(decl) != mVars.end();
	}

	int64_t getDeclVal(clang::Decl * decl) {
		assert (mVars.find(decl) != mVars.end());
		return mVars.find(decl)->second;
	}

	//Stmts
	void bindStmt(clang::Stmt * stmt, int64_t val) {
		mExprs[stmt] = val;
	}

	bool findStmtVal(clang::Stmt * stmt) {
		return mExprs.find(stmt) != mExprs.end();
	}

	int64_t getStmtVal(clang::Stmt * stmt) {
		assert (mExprs.find(stmt) != mExprs.end());
		return mExprs[stmt];
	}

	//Pointers
	void bindPointer(clang::Stmt * stmt, int64_t val) {
		mPointers[stmt] = val;
	}

	int64_t getPointerVal(clang::Stmt * stmt) {
		assert (mPointers.find(stmt) != mPointers.end());
		return mPointers[stmt];
	}

	//PC
	void setPC(clang::Stmt * stmt) {
		mPC = stmt;
	}

	clang::Stmt * getPC() {
		return mPC;
	}

	//returnVal
	void setReturn(int r) {
		returnVal = r;
	}

	int getReturn() {
		return returnVal;
	}
};

/// Heap maps address to a value
class Heap {
	std::map<clang::Decl*, int64_t> mVars;
	std::map<clang::Stmt*, int64_t> mExprs;

public:
	Heap() : mVars() {}

	void bindDecl(clang::Decl* decl, int64_t val) {
		mVars[decl] = val;
	}

	int64_t getDeclVal(clang::Decl * decl) {
		assert (mVars.find(decl) != mVars.end());
		return mVars.find(decl)->second;
	}

	void bindStmt(clang::Stmt * stmt, int64_t val) {
		mExprs[stmt] = val;
	}

	int64_t getStmtVal(clang::Stmt * stmt) {
		assert (mExprs.find(stmt) != mExprs.end());
		return mExprs[stmt];
	}

	int64_t Malloc(int64_t size) ;
	void Free (int64_t addr) ;
	void Update(int64_t addr, int64_t val) ;
	int64_t get(int64_t addr);
};

class Environment {
	std::vector<StackFrame> mStack;
	Heap mHeap;

	clang::FunctionDecl * mFree;				/// Declartions to the built-in functions
	clang::FunctionDecl * mMalloc;
	clang::FunctionDecl * mInput;
	clang::FunctionDecl * mOutput;
	clang::FunctionDecl * mEntry;

public:
	/// Get the declartions to the built-in functions
	Environment() : mStack(), mHeap(), mFree(NULL), mMalloc(NULL), mInput(NULL), mOutput(NULL), mEntry(NULL) {}

	/// Initialize the Environment
	void init(clang::TranslationUnitDecl * unit, const clang::ASTContext& context);

	clang::FunctionDecl * getEntry() { return mEntry; }

	int64_t getStmtVal(clang::Stmt * stmt) {
		int64_t val;
		if (mStack.back().findStmtVal(stmt)) {
			val = mStack.back().getStmtVal(stmt);
		} else {
			val = mHeap.getStmtVal(stmt);
		}
		return val;
	}

	int64_t getDeclVal(clang::Decl * decl) {
		int64_t val;
		if (mStack.back().findDeclVal(decl)) {
			val = mStack.back().getDeclVal(decl);
		} else {
			val = mHeap.getDeclVal(decl);
		}
		return val;
	}

	void binop(clang::BinaryOperator * bop);
	void unop(clang::UnaryOperator * uop);
	void paren(clang::ParenExpr * paren);
	void literal(clang::IntegerLiteral * literal, const clang::ASTContext& context);
	void decl(clang::DeclStmt * declstmt, const clang::ASTContext& context);
	void declref(clang::DeclRefExpr * declref);
	void cast(clang::CastExpr * castexpr);
	void array(clang::ArraySubscriptExpr * array);
	void ueotte(clang::UnaryExprOrTypeTraitExpr * ueotte);
	bool beforeCall(clang::CallExpr * callexpr);
	void afterCall(clang::CallExpr * callexpr);
	bool cond(clang::Expr * cond);
	void returnStmt(clang::ReturnStmt * restmt);
};
