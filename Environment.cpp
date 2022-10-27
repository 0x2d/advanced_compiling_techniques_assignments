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
			int val = 0;
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
	mStack.back().setPC(mEntry);
}

//返回值为false，则为内建函数；返回值为true，则为自定义函数，需要执行afterCall
bool Environment::beforeCall(clang::CallExpr * callexpr) {
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
	} else if (callee == mMalloc) {
		clang::Expr * decl = callexpr->getArg(0);
		val = getStmtVal(decl);
		void * p = malloc(val);
		memset(p, 0, val);
		mStack.back().bindStmt(callexpr, (int64_t)p);
		return false;
	} else if (callee == mFree) {
		clang::Expr * decl = callexpr->getArg(0);
		val = getStmtVal(decl);
		free((void *)val);
		return false;
	} else {
		int numArgs = callexpr->getNumArgs();
		int64_t vals[numArgs];
		clang::Expr * decls[numArgs];
		clang::ParmVarDecl * params[numArgs];
		//指向函数定义，而非函数声明
		if (callee->isDefined()) {
			callee = callee->getDefinition();
		}
		for (int i = 0; i < numArgs; i++) {
			decls[i] = callexpr->getArg(i);
			vals[i] = getStmtVal(decls[i]);
			params[i] = callee->getParamDecl(i);
		}
		mStack.push_back(StackFrame());
		mStack.back().setPC(callee);
		for (int i = 0; i < numArgs; i++) {
			mStack.back().bindDecl(params[i], vals[i]);
			#ifdef _DEBUG
				std::cout << " Pushing function param " << vals[i] << std::endl;
			#endif
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
			//若左值为数组，左节点必然为ArraySubscriptExpr，因此在ArraySubscriptExpr阶段分别存储值和地址
			int64_t address = mStack.back().getPointerVal(array);
			clang::QualType type = array->getType();
			if (type->isIntegerType()) {
				*((int *)address) = (int)val;
			} else if (type->isPointerType()) {
				*((int64_t *)address) = val;
			}
		} else if (clang::UnaryOperator * pointer = clang::dyn_cast<clang::UnaryOperator>(left)) {
			//若左值为指针，左节点必然为UnaryOperator，因此在UnaryOperator阶段分别存储值和地址
			int64_t address = mStack.back().getPointerVal(pointer);
			clang::QualType type = pointer->getType();
			if (type->isIntegerType()) {
				*((int *)address) = (int)val;
			} else if (type->isPointerType()) {
				*((int64_t *)address) = val;
			}
		}
	} else {
		int64_t leftVal = getStmtVal(left);
		int64_t rightVal = getStmtVal(right);
		clang::QualType leftType = left->getType();
		clang::QualType rightType = right->getType();
		if (leftType->isPointerType() && rightType->isIntegerType()) {
			const clang::PointerType * pType = clang::dyn_cast<clang::PointerType>(leftType.getTypePtr());
			clang::QualType peType = pType->getPointeeType();
			if (peType->isIntegerType()) {
				rightVal *= sizeof(int);
			} else if (peType->isPointerType()) {
				rightVal *= sizeof(int64_t);
			}
		} else if (rightType->isPointerType() && leftType->isIntegerType()) {
			const clang::PointerType * pType = clang::dyn_cast<clang::PointerType>(rightType.getTypePtr());
			clang::QualType peType = pType->getPointeeType();
			if (peType->isIntegerType()) {
				leftVal *= sizeof(int);
			} else if (peType->isPointerType()) {
				leftVal *= sizeof(int64_t);
			}
		}
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
			case clang::BO_GE :
				#ifdef _DEBUG
					std::cout << " Result " << (leftVal >= rightVal) << std::endl;
				#endif
				mStack.back().bindStmt(bop, leftVal >= rightVal);
				break;
			case clang::BO_LE :
				#ifdef _DEBUG
					std::cout << " Result " << (leftVal <= rightVal) << std::endl;
				#endif
				mStack.back().bindStmt(bop, leftVal <= rightVal);
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
		case clang::UO_Deref :
			clang::QualType type = uop->getType();
			if (type->isIntegerType()) {
				mStack.back().bindStmt(uop, *((int *)subVal));
			} else if (type->isPointerType()) {
				mStack.back().bindStmt(uop, *((int64_t *)subVal));
			}
			mStack.back().bindPointer(uop, subVal);
			break;
	}
}

void Environment::paren(clang::ParenExpr * paren) {
	mStack.back().bindStmt(paren, getStmtVal(paren->getSubExpr()));
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
          		int size = array->getSize().getSExtValue();
				clang::QualType type = array->getElementType();
				void * arrayAddress = NULL;
				if (type->isIntegerType()) {
					arrayAddress = malloc(size*sizeof(int));
					for (int i = 0; i < size; i++) {
						((int *)arrayAddress)[i] = 0;
					}
				} else if (type->isPointerType()) {
          			arrayAddress = malloc(size*sizeof(int64_t));
					for (int i = 0; i < size; i++) {
						((int64_t *)arrayAddress)[i] = 0;
					}
				}
				mStack.back().bindDecl(vardecl, (int64_t)arrayAddress);
			} else if (type->isPointerType() && !type->isFunctionPointerType()) {
				int64_t * val = NULL;
				if (vardecl->hasInit()) {
					clang::Expr * init = vardecl->getInit();
					llvm::APSInt intResult = llvm::APSInt();
					if (init->isIntegerConstantExpr(intResult, context)){
						val = (int64_t *)malloc(sizeof(int64_t));
						*val = intResult.getExtValue();
					}
				}
				mStack.back().bindDecl(vardecl, (int64_t)val);
			}
		}
	}
}

void Environment::declref(clang::DeclRefExpr * declref) {
	int64_t val;
	clang::Decl * decl;
	clang::QualType type = declref->getType();
	if (type->isIntegerType() || type->isArrayType() || (type->isPointerType() && !type->isFunctionPointerType())) {
		decl = declref->getFoundDecl();
		val = getDeclVal(decl);
		mStack.back().bindStmt(declref, val);
	}
}

void Environment::cast(clang::CastExpr * castexpr) {
	int64_t val;
	clang::Expr * expr = castexpr->getSubExpr();
	clang::QualType type = castexpr->getType();
	if (type->isIntegerType() || (type->isPointerType() && !type->isFunctionPointerType())) {
		val = getStmtVal(expr);
		mStack.back().bindStmt(castexpr, val);
	}
}

void Environment::array(clang::ArraySubscriptExpr * array) {
	clang::Expr * base = array->getBase();
	clang::Expr * idx = array->getIdx();
	int64_t val, address;
	clang::QualType type = base->getType();
	const clang::PointerType * pType= clang::dyn_cast<clang::PointerType>(type.getTypePtr());
	clang::QualType peType = pType->getPointeeType();
	if (peType->isIntegerType()) {
		address = getStmtVal(base) + getStmtVal(idx) * sizeof(int);
		val = *((int *)address);
	} else if (peType->isPointerType()) {
		address = getStmtVal(base) + getStmtVal(idx) * sizeof(int64_t);
		val = *((int64_t *)address);
	}
	mStack.back().bindStmt(array, val);
	mStack.back().bindPointer(array, address);
}

void Environment::literal(clang::IntegerLiteral * literal, const clang::ASTContext& context) {
	int64_t val;
	llvm::APSInt intResult;
	literal->isIntegerConstantExpr(intResult, context);
	val = intResult.getExtValue();
	mHeap.bindStmt(literal, val);
}

void Environment::ueotte(clang::UnaryExprOrTypeTraitExpr * ueotte) {
	int64_t val = 0;
	clang::UnaryExprOrTypeTrait kind = ueotte->getKind();
	//目前只考虑sizeof
	if (kind == clang::UETT_SizeOf) {
		clang::QualType type = ueotte->getArgumentType();
		if (type->isIntegerType()) {
			val = 4;
		} else if (type->isPointerType()) {
			val = 8;
		}
	}
	mStack.back().bindStmt(ueotte, val);
}
