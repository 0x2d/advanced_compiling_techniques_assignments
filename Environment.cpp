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
			int64_t val = 0;
			if (vdecl->hasInit()) {
				clang::Expr * init = vdecl->getInit();
				llvm::APSInt intResult = llvm::APSInt();
				if (init->isIntegerConstantExpr(intResult, context)){
					val = intResult.getExtValue();	
				}
			}
			mHeap.bindDecl(vdecl, val);
		}
	}
	mStack.push_back(StackFrame());
}

bool Environment::beforeCall(clang::CallExpr * callexpr) {
	mStack.back().setPC(callexpr);
	int64_t val = 0;
	clang::FunctionDecl * callee = callexpr->getDirectCallee();
	if (callee == mInput) {
		llvm::outs() << "Please Input an Integer Value : ";
		scanf("%ld", &val);
		mStack.back().bindStmt(callexpr, val);
		return false;
	} else if (callee == mOutput) {
		clang::Expr * decl = callexpr->getArg(0);
		val = getStmtVal(decl);
		llvm::outs() << val;
		return false;
	} else {
		int numArgs = callexpr->getNumArgs();
		int64_t vals[numArgs];
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
	return getStmtVal(cond);
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
		int64_t val = getStmtVal(right);
		#ifdef _DEBUG
			std::cout << " Right value " << val << std::endl;
		#endif
		mStack.back().bindStmt(left, val);
		if (clang::DeclRefExpr * declexpr = clang::dyn_cast<clang::DeclRefExpr>(left)) {
			clang::Decl * decl = declexpr->getFoundDecl();
			mStack.back().bindDecl(decl, val);
		} else if (clang::ArraySubscriptExpr * array = clang::dyn_cast<clang::ArraySubscriptExpr>(left)) {
			int64_t address = mStack.back().getPointerVal(array);
			*((int64_t *)address) = val;
		}
	} else {
		int64_t leftVal = getStmtVal(left);
		int64_t rightVal = getStmtVal(right);
		#ifdef _DEBUG
			std::cout << " Left value " << leftVal << " Right value " << rightVal;
		#endif
		switch (opc) {
			case clang::BO_GT :
				#ifdef _DEBUG
					std::cout << " Result " << (leftVal > rightVal) << std::endl;
				#endif
				mStack.back().bindStmt(bop, leftVal > rightVal);
				break;
			case clang::BO_LT :
				#ifdef _DEBUG
					std::cout << " Result " << (leftVal < rightVal) << std::endl;
				#endif
				mStack.back().bindStmt(bop, leftVal < rightVal);
				break;
			case clang::BO_EQ :
				#ifdef _DEBUG
					std::cout << " Result " << (leftVal == rightVal) << std::endl;
				#endif
				mStack.back().bindStmt(bop, leftVal == rightVal);
				break;
			case clang::BO_Sub :
				#ifdef _DEBUG
					std::cout << " Result " << (leftVal - rightVal) << std::endl;
				#endif
				mStack.back().bindStmt(bop, leftVal - rightVal);
				break;
			case clang::BO_Add :
				#ifdef _DEBUG
					std::cout << " Result " << (leftVal + rightVal) << std::endl;
				#endif
				mStack.back().bindStmt(bop, leftVal + rightVal);
				break;
			case clang::BO_Mul :
				#ifdef _DEBUG
					std::cout << " Result " << (leftVal * rightVal) << std::endl;
				#endif
				mStack.back().bindStmt(bop, leftVal * rightVal);
				break;
			case clang::BO_Div :
				#ifdef _DEBUG
					std::cout << " Result " << (leftVal / rightVal) << std::endl;
				#endif
				mStack.back().bindStmt(bop, leftVal / rightVal);
				break;
		}
	}
}

void Environment::unop(clang::UnaryOperator * uop) {
	clang::Expr * sub = uop->getSubExpr();
	clang::UnaryOperator::Opcode opc = uop->getOpcode();
	int64_t subVal = getStmtVal(sub);
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
			clang::QualType type = vardecl->getType();
			if (type->isIntegerType()) {
				int64_t val = 0;
				if (vardecl->hasInit()) {
					clang::Expr * init = vardecl->getInit();
					llvm::APSInt intResult = llvm::APSInt();
					if (init->isIntegerConstantExpr(intResult, context)){
						val = intResult.getExtValue();
					}
				}
				mStack.back().bindDecl(vardecl, val);
			} else if (type->isArrayType()) {
				const clang::ConstantArrayType *array = clang::dyn_cast<clang::ConstantArrayType>(type.getTypePtr());
          		int64_t size = array->getSize().getSExtValue();
          		int64_t * arrayAddress = new int64_t[size];
				for (int i = 0; i < size; i++) {
					arrayAddress[i] = 0;
				}
				mStack.back().bindDecl(vardecl, (int64_t)arrayAddress);
			}
		}
	}
}

void Environment::declref(clang::DeclRefExpr * declref) {
	mStack.back().setPC(declref);
	int64_t val;
	clang::Decl * decl;
	clang::QualType type = declref->getType();
	if (type->isIntegerType() || type->isArrayType()) {
		decl = declref->getFoundDecl();
		val = getDeclVal(decl);
		mStack.back().bindStmt(declref, val);
	}
}

void Environment::cast(clang::CastExpr * castexpr) {
	mStack.back().setPC(castexpr);
	int64_t val;
	clang::Expr * expr = castexpr->getSubExpr();
	clang::QualType type = castexpr->getType();
	if (type->isIntegerType()) {
		val = getStmtVal(expr);
		mStack.back().bindStmt(castexpr, val);
	} else if (type->isPointerType() && !type->isFunctionPointerType()) {
		val = getStmtVal(expr);
		mStack.back().bindStmt(castexpr, val);
	}
}

void Environment::array(clang::ArraySubscriptExpr * array) {
	clang::Expr * base = array->getBase();
	clang::Expr * idx = array->getIdx();
	int64_t address = getStmtVal(base) + getStmtVal(idx);
	mStack.back().bindStmt(array, *((int64_t *)address));
	mStack.back().bindPointer(array, address);
}

void Environment::literal(clang::IntegerLiteral * literal, const clang::ASTContext& context) {
	int64_t val;
	llvm::APSInt intResult;
	literal->isIntegerConstantExpr(intResult, context);
	val = intResult.getExtValue();
	mHeap.bindStmt(literal, val);
}
