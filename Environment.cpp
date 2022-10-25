#include "Environment.h"

void Environment::init(clang::TranslationUnitDecl * unit, const clang::ASTContext& context) {
	for (clang::TranslationUnitDecl::decl_iterator i =unit->decls_begin(), e = unit->decls_end(); i != e; ++ i) {
		if (clang::FunctionDecl * fdecl = clang::dyn_cast<clang::FunctionDecl>(*i) ) {
			if (fdecl->getName().equals("FREE")) mFree = fdecl;
			else if (fdecl->getName().equals("MALLOC")) mMalloc = fdecl;
			else if (fdecl->getName().equals("GET")) mInput = fdecl;
			else if (fdecl->getName().equals("PRINT")) mOutput = fdecl;
			else if (fdecl->getName().equals("main")) mEntry = fdecl;
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

bool Environment::beforeCall(clang::CallExpr * callexpr) {
	mStack.back().setPC(callexpr);
	int val = 0;
	clang::FunctionDecl * callee = callexpr->getDirectCallee();
	if (callee == mInput) {
		llvm::outs() << "Please Input an Integer Value : ";
		scanf("%d", &val);
		mStack.back().bindStmt(callexpr, val);
		return false;
	} else if (callee == mOutput) {
		clang::Expr * decl = callexpr->getArg(0);
		val = getStmtVal(decl);
		llvm::outs() << val;
		return false;
	} else {
		int numArgs = callexpr->getNumArgs();
		int vals[numArgs];
		clang::Expr * decls[numArgs];
		clang::ParmVarDecl * params[numArgs];
		for (int i = 0; i < numArgs; i++) {
			decls[i] = callexpr->getArg(i);
			vals[i] = getStmtVal(decls[i]);
			params[i] = callee->getParamDecl(i);
		}
		mStack.push_back(StackFrame());
		for (int i = 0; i < numArgs; i++) {
			mStack.back().bindDecl(params[i], vals[i]);
		}
		return true;
	}
}

void Environment::afterCall(clang::CallExpr * callexpr) {
	clang::FunctionDecl * callee = callexpr->getDirectCallee();
	(*(mStack.end()-2)).bindStmt(callexpr, mStack.back().getReturn());
	mStack.pop_back();
}

bool Environment::cond(clang::Expr * cond) {
	return mStack.back().getStmtVal(cond);
}

void Environment::returnStmt(clang::ReturnStmt * restmt) {
	mStack.back().setReturn(getStmtVal(restmt->getRetValue()));
}

///home/ouyang/llvm-project/clang/include/clang/AST/OperationKinds.def
void Environment::binop(clang::BinaryOperator * bop) {
	clang::Expr * left = bop->getLHS();
	clang::Expr * right = bop->getRHS();
	clang::BinaryOperator::Opcode opc = bop->getOpcode();
	if (bop->isAssignmentOp()) {
		int val = getStmtVal(right);
		mStack.back().bindStmt(left, val);
		if (clang::DeclRefExpr * declexpr = clang::dyn_cast<clang::DeclRefExpr>(left)) {
			clang::Decl * decl = declexpr->getFoundDecl();
			mStack.back().bindDecl(decl, val);
		}
	} else {
		int leftVal = getStmtVal(left);
		int rightVal = getStmtVal(right);
		switch (opc) {
			case clang::BO_GT :
				mStack.back().bindStmt(bop, leftVal > rightVal);
				break;
			case clang::BO_LT :
				mStack.back().bindStmt(bop, leftVal < rightVal);
				break;
			case clang::BO_EQ :
				mStack.back().bindStmt(bop, leftVal == rightVal);
				break;
			case clang::BO_Sub :
				mStack.back().bindStmt(bop, leftVal - rightVal);
				break;
			case clang::BO_Add :
				mStack.back().bindStmt(bop, leftVal + rightVal);
				break;
			case clang::BO_Mul :
				mStack.back().bindStmt(bop, leftVal * rightVal);
				break;
		}
	}
}

void Environment::unop(clang::UnaryOperator * uop) {
	clang::Expr * sub = uop->getSubExpr();
	clang::UnaryOperator::Opcode opc = uop->getOpcode();
	int subVal = getStmtVal(sub);
	switch (opc) {
		case clang::UO_Minus :
			mStack.back().bindStmt(uop, -subVal);
			break;
	}
}

void Environment::decl(clang::DeclStmt * declstmt, const clang::ASTContext& context) {
	for (clang::DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end(); it != ie; ++ it) {
		clang::Decl * decl = *it;
		if (clang::VarDecl * vardecl = clang::dyn_cast<clang::VarDecl>(decl)) {
			int val = 0;
			if (vardecl->hasInit()) {
				clang::Expr * init = vardecl->getInit();
				llvm::APSInt intResult = llvm::APSInt();
				if (init->isIntegerConstantExpr(intResult, context)){
					val = intResult.getExtValue();
				}
			}
			mStack.back().bindDecl(vardecl, val);
		}
	}
}

void Environment::declref(clang::DeclRefExpr * declref) {
	mStack.back().setPC(declref);
	if (declref->getType()->isIntegerType()) {
		clang::Decl * decl = declref->getFoundDecl();
		int val = getDeclVal(decl);
		mStack.back().bindStmt(declref, val);
	}
}

void Environment::cast(clang::CastExpr * castexpr) {
	mStack.back().setPC(castexpr);
	if (castexpr->getType()->isIntegerType()) {
		clang::Expr * expr = castexpr->getSubExpr();
		int val = getStmtVal(expr);
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
