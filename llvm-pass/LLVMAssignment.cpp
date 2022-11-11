//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include <llvm/Support/CommandLine.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/ToolOutputFile.h>

#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/InstIterator.h"
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>

#include <set>
#include <stdexcept>

// DEBUG宏定义
// #define _DEBUG

static llvm::ManagedStatic<llvm::LLVMContext> GlobalContext;
static llvm::LLVMContext &getGlobalContext() { return *GlobalContext; }
/* In LLVM 5.0, when  -O0 passed to clang , the functions generated with clang will
 * have optnone attribute which would lead to some transform passes disabled, like mem2reg.
 */
struct EnableFunctionOptPass: public llvm::FunctionPass {
	static char ID;
	EnableFunctionOptPass():llvm::FunctionPass(ID){}
	bool runOnFunction(llvm::Function & F) override{
		if(F.hasFnAttribute(llvm::Attribute::OptimizeNone)){
			F.removeFnAttr(llvm::Attribute::OptimizeNone);
		}
		return true;
	}
};

char EnableFunctionOptPass::ID=0;

///!TODO TO BE COMPLETED BY YOU FOR ASSIGNMENT 2
///Updated 11/10/2017 by fargo: make all functions
///processed by mem2reg before this pass.
struct FuncPtrPass : public llvm::ModulePass {
	static char ID; // Pass identification, replacement for typeid
	std::set<std::string> funcNames;
	//用于记录嵌套调用的级数，非嵌套调用时，应记录函数名；嵌套调用时，应获取函数的返回值
	int callNest;
	int iter = 0;

	FuncPtrPass() : llvm::ModulePass(ID) {}

	void value(llvm::Value * value) {
		if (llvm::isa<llvm::CallInst>(value)) {
			callInstAgain(llvm::dyn_cast<llvm::CallInst>(value));
		} else if (llvm::isa<llvm::Function>(value)) {
			function(llvm::dyn_cast<llvm::Function>(value));
		} else if (llvm::isa<llvm::PHINode>(value)) {
			phiNode(llvm::dyn_cast<llvm::PHINode>(value));
		} else if (llvm::isa<llvm::Argument>(value)) {
			argument(llvm::dyn_cast<llvm::Argument>(value));
		}
	}

	//非首次访问CallInst，意味着发生了嵌套调用
	void callInstAgain(llvm::CallInst * callinst) {
		iter++;
		if (iter > 50) {	//这个数是随便设的
			throw std::exception();	//真不会做了，救命
		}
	#ifdef _DEBUG
		llvm::errs() << "	CallInstAgain" << "\n";
	#endif
		llvm::Function * func = callinst->getCalledFunction();
		if (func) {
			callNest++;
			function(func);
		} else {
			llvm::Value * operand = callinst->getCalledOperand();
			if (llvm::isa<llvm::PHINode>(operand)) {
				llvm::PHINode * phinode = llvm::dyn_cast<llvm::PHINode>(operand);
				int numPhi = phinode->getNumIncomingValues();
				for (int i = 0; i < numPhi; i++) {
					llvm::Value * incomeV = phinode->getIncomingValue(i);
					callNest++;
					function(llvm::dyn_cast<llvm::Function>(incomeV));
				}
			} else if (llvm::isa<llvm::Argument>(operand)) {
				callNest++;
				argument(llvm::dyn_cast<llvm::Argument>(operand));
			}
		}
	}

	//访问函数形参，应根据函数的def-use链追溯到函数实参
	void argument(llvm::Argument * argument) {
	#ifdef _DEBUG
		llvm::errs() << "	Argument" << "\n";
	#endif
		unsigned argNo = argument->getArgNo();
		llvm::Function * parent = argument->getParent();
		for (llvm::Value::user_iterator ui = parent->user_begin(), ue = parent->user_end(); ui != ue; ++ui) {
			llvm::User * user = *ui;
			
			if (llvm::isa<llvm::CallInst>(user)) {
			#ifdef _DEBUG
				llvm::errs() << "		CallInstUser" << "\n";
			#endif
				llvm::CallInst * callinst = llvm::dyn_cast<llvm::CallInst>(user);
				llvm::Function * called = callinst->getCalledFunction();
				if (called == parent) {
					llvm::Value * operand = callinst->getOperand(argNo);
					value(operand);
				} else {	//此时user调用的并不是我们查找的函数，真正的函数调用发生在该函数内部
					for (llvm::inst_iterator I = llvm::inst_begin(called), E = llvm::inst_end(called); I != E; ++I) {
						if (llvm::isa<llvm::ReturnInst>(*I)) {
							llvm::ReturnInst * ret =  llvm::dyn_cast<llvm::ReturnInst>(&*I);
							llvm::Value * retV = ret->getReturnValue();
							if (llvm::isa<llvm::CallInst>(retV)) {
								llvm::CallInst * ref = llvm::dyn_cast<llvm::CallInst>(retV);
								llvm::Value * operand = ref->getOperand(argNo);
								value(operand);
							}
						}
					}
				}
			} else if (llvm::isa<llvm::PHINode>(user)) {
			#ifdef _DEBUG
				llvm::errs() << "		PHINodeUser" << "\n";
			#endif
				for (llvm::Value::user_iterator ui = user->user_begin(), ue = user->user_end(); ui != ue; ++ui) {
					llvm::User * phiuser = *ui;
					if (llvm::isa<llvm::CallInst>(phiuser)) {
						llvm::CallInst * callinst = llvm::dyn_cast<llvm::CallInst>(phiuser);
						llvm::Value * called = callinst->getCalledOperand();
						if (called == user) {
							llvm::Value * operand = callinst->getOperand(argNo);
							value(operand);
						} else {	//此时user调用的并不是我们查找的函数，真正的函数调用发生在该函数内部
							llvm::Function * calledf = callinst->getCalledFunction();
							for (llvm::inst_iterator I = llvm::inst_begin(calledf), E = llvm::inst_end(calledf); I != E; ++I) {
								if (llvm::isa<llvm::ReturnInst>(*I)) {
									llvm::ReturnInst * ret =  llvm::dyn_cast<llvm::ReturnInst>(&*I);
									llvm::Value * retV = ret->getReturnValue();
									if (llvm::isa<llvm::CallInst>(retV)) {
										llvm::CallInst * ref = llvm::dyn_cast<llvm::CallInst>(retV);
										llvm::Value * operand = ref->getOperand(argNo);
										value(operand);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	//访问函数时，需根据是否为嵌套调用，确定执行操作
	void function(llvm::Function * func) {
		if (callNest == 0) {
		#ifdef _DEBUG
			llvm::errs() << "	Function "<< func->getName().str() << "\n";
		#endif
			funcNames.insert(func->getName().str());
		} else if (callNest > 0) {
		#ifdef _DEBUG
			llvm::errs() << "	FunctionReturn" << "\n";
		#endif
			for (llvm::inst_iterator I = llvm::inst_begin(func), E = llvm::inst_end(func); I != E; ++I) {
				if (llvm::isa<llvm::ReturnInst>(*I)) {
					llvm::ReturnInst * ret =  llvm::dyn_cast<llvm::ReturnInst>(&*I);
					llvm::Value * retV = ret->getReturnValue();
					callNest--;
					value(retV);
				}
			}
		}
	}

	//PHI结点的所有可能值均应考虑
	void phiNode(llvm::PHINode * phinode) {
	#ifdef _DEBUG
		llvm::errs() << "	PHINode" << "\n";
	#endif
		bool nestFlag = false;
		if (callNest > 0) {
			callNest--;
			nestFlag = true;
		}
		int numPhi = phinode->getNumIncomingValues();
		for (int i = 0; i < numPhi; i++) {
			llvm::Value * incomeV = phinode->getIncomingValue(i);
			if (nestFlag) callNest++;
			value(incomeV);
		}
	}

	//首次访问CallInst，负责最终输出
	void callInst(llvm::CallInst * callinst) {
		int lineno = callinst->getDebugLoc().getLine();
		llvm::Function * func = callinst->getCalledFunction();
		if (func) {
			llvm::errs() << lineno << " : " << func->getName().str() << "\n";
		} else {
			funcNames.clear();
			callNest = 0;
			iter = 0;
			llvm::Value * operand = callinst->getCalledOperand();
			try {
				value(operand);
			} catch (const std::exception & e) {}
			llvm::errs() << lineno << " : ";
			int index = 0;
			for (std::string s: funcNames) {
				if (index == 0) {
					llvm::errs() << s;
				} else {
					llvm::errs() << ", " << s;
				}
				index++;
			}
			llvm::errs() << "\n";
		}
	}

	bool runOnModule(llvm::Module &M) override {
	#ifdef _DEBUG
		llvm::errs() << "Hello: ";
		llvm::errs().write_escaped(M.getName()) << '\n';
		M.print(llvm::errs(), nullptr);
		// 该函数在release版的LLVM中没有开启，使用以上函数代替
		// M.dump();
		llvm::errs()<<"------------------------------\n";
	#endif
		for (llvm::Function &F : M) {
			for (llvm::inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
				//需要排除llvm.dbg.value
				if (llvm::isa<llvm::CallInst>(*I) && !llvm::isa<llvm::DbgInfoIntrinsic>(*I)) {
				#ifdef _DEBUG
					llvm::errs() << *I << "\n";
				#endif
					callInst(llvm::dyn_cast<llvm::CallInst>(&*I));
				}
			}
		}
		return false;
	}
};

char FuncPtrPass::ID = 0;
static llvm::RegisterPass<FuncPtrPass> X("funcptrpass", "Print function call instruction");

static llvm::cl::opt<std::string>
InputFilename(llvm::cl::Positional,
              llvm::cl::desc("<filename>.bc"),
              llvm::cl::init(""));

int main(int argc, char **argv) {
	llvm::LLVMContext &Context = getGlobalContext();
	llvm::SMDiagnostic Err;
	// Parse the command line to read the Inputfilename
	llvm::cl::ParseCommandLineOptions(argc, argv,
								"FuncPtrPass \n My first LLVM too which does not do much.\n");


	// Load the input module
	std::unique_ptr<llvm::Module> M = parseIRFile(InputFilename, Err, Context);
	if (!M) {
		Err.print(argv[0], llvm::errs());
		return 1;
	}

	llvm::legacy::PassManager Passes;
		
	///Remove functions' optnone attribute in LLVM5.0
	Passes.add(new EnableFunctionOptPass());
	///Transform it to SSA
	Passes.add(llvm::createPromoteMemoryToRegisterPass());

	/// Your pass to print Function and Call Instructions
	Passes.add(new FuncPtrPass());
	Passes.run(*M.get());
}
