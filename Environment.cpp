#include "Environment.h"

void Environment::init(clang::TranslationUnitDecl * unit, const clang::ASTContext& context) {
	for (clang::TranslationUnitDecl::decl_iterator i =unit->decls_begin(), e = unit->decls_end(); i != e; ++ i) {
		if (clang::FunctionDecl * fdecl = clang::dyn_cast<clang::FunctionDecl>(*i) ) {
			if (fdecl->getName().equals("FREE")) mFree = fdecl;
			else if (fdecl->getName().equals("MALLOC")) mMalloc = fdecl;
			else if (fdecl->getName().equals("GET")) mInput = fdecl;
			else if (fdecl->getName().equals("PRINT")) mOutput = fdecl;
			else if (fdecl->getName().equals("main")) mEntry = fdecl;
			else {
				mFuncs.push_back(fdecl);
			}
		} else if (clang::VarDecl * vdecl = clang::dyn_cast<clang::VarDecl>(*i)) {
			clang::Expr * init = vdecl->getInit();
			llvm::APSInt intResult = llvm::APSInt();
			int val;
			if (init->isIntegerConstantExpr(intResult, context)){
				val = intResult.getExtValue();
			}
			mHeap.bindDecl(vdecl, val);
		}
	}
	mStack.push_back(StackFrame());
}

void Environment::call(clang::CallExpr * callexpr, InterpreterVisitor * mVisitor) {
	mStack.back().setPC(callexpr);
	int val = 0;
	clang::FunctionDecl * callee = callexpr->getDirectCallee();
	if (callee == mInput) {
		llvm::outs() << "Please Input an Integer Value : ";
		scanf("%d", &val);
		mStack.back().bindStmt(callexpr, val);
	} else if (callee == mOutput) {
		clang::Expr * decl = callexpr->getArg(0);
		val = mStack.back().getStmtVal(decl);
		llvm::outs() << val;
	} else {
		clang::Expr * decl = callexpr->getArg(0);
		val = mStack.back().getStmtVal(decl);
		clang::ParmVarDecl * param = callee->getParamDecl(0);
		mStack.push_back(StackFrame());
		mStack.back().bindDecl(param, val);
		mVisitor->VisitStmt(callee->getBody());
		(*(mStack.end()-2)).bindStmt(callexpr, mStack.back().getReturn());
		mStack.pop_back();
	}
}

void Environment::ifstmt(clang::IfStmt * ifstmt) {
	//todo
}

void Environment::binop(clang::BinaryOperator * bop, const clang::ASTContext& context) {
	clang::Expr * left = bop->getLHS();
	clang::Expr * right = bop->getRHS();
	if (bop->isAssignmentOp()) {
		int val;
		if (mStack.back().findStmtVal(right)) {
			val = mStack.back().getStmtVal(right);
		} else {
			val = mHeap.getStmtVal(right);
		}
		mStack.back().bindStmt(left, val);
		if (clang::DeclRefExpr * declexpr = clang::dyn_cast<clang::DeclRefExpr>(left)) {
			clang::Decl * decl = declexpr->getFoundDecl();
			mStack.back().bindDecl(decl, val);
		}
	}
}

void Environment::decl(clang::DeclStmt * declstmt) {
	for (clang::DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end(); it != ie; ++ it) {
		clang::Decl * decl = *it;
		if (clang::VarDecl * vardecl = clang::dyn_cast<clang::VarDecl>(decl)) {
			mStack.back().bindDecl(vardecl, 0);
		}
	}
}

void Environment::declref(clang::DeclRefExpr * declref) {
	mStack.back().setPC(declref);
	if (declref->getType()->isIntegerType()) {
		clang::Decl* decl = declref->getFoundDecl();
		int val;
		if (mStack.back().findDeclVal(decl)) {
			val = mStack.back().getDeclVal(decl);
		} else {
			val = mHeap.getDeclVal(decl);
		}
		mStack.back().bindStmt(declref, val);
	}
}

void Environment::cast(clang::CastExpr * castexpr) {
	mStack.back().setPC(castexpr);
	if (castexpr->getType()->isIntegerType()) {
		clang::Expr * expr = castexpr->getSubExpr();
		int val = mStack.back().getStmtVal(expr);
		mStack.back().bindStmt(castexpr, val );
	}
}

void Environment::literal(clang::IntegerLiteral * literal, const clang::ASTContext& context) {
	int val;
	llvm::APSInt intResult;
	literal->isIntegerConstantExpr(intResult, context);
	val = intResult.getExtValue();
	mHeap.bindStmt(literal, val);
}
