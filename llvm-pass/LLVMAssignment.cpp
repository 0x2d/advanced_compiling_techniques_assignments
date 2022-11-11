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

	void callInstAgain(llvm::CallInst * callinst) {
	#ifdef _DEBUG
		llvm::errs() << "	CallInstAgain" << "\n";
	#endif
		llvm::Function * func = callinst->getCalledFunction();
		if (func) {
			functionReturn(func);
		} else {
			llvm::Value * operand = callinst->getCalledOperand();
			if (llvm::isa<llvm::PHINode>(operand)) {
				llvm::PHINode * phinode = llvm::dyn_cast<llvm::PHINode>(operand);
				int numPhi = phinode->getNumIncomingValues();
				for (int i = 0; i < numPhi; i++) {
					llvm::Value * incomeV = phinode->getIncomingValue(i);
					functionReturn(llvm::dyn_cast<llvm::Function>(incomeV));
				}
			} else if (llvm::isa<llvm::Argument>(operand)) {
				//
			}
		}
	}

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
				llvm::errs() << "	CallInstUser" << "\n";
			#endif
				llvm::CallInst * callinst = llvm::dyn_cast<llvm::CallInst>(user);
				llvm::Value * operand = callinst->getOperand(argNo);
				value(operand);
			} else if (llvm::isa<llvm::PHINode>(user)) {
			#ifdef _DEBUG
				llvm::errs() << "	PHINodeUser" << "\n";
			#endif
				for (llvm::Value::user_iterator ui = user->user_begin(), ue = user->user_end(); ui != ue; ++ui) {
					llvm::User * phiuser = *ui;
					if (llvm::isa<llvm::CallInst>(phiuser)) {
						llvm::CallInst * callinst = llvm::dyn_cast<llvm::CallInst>(phiuser);
						llvm::Value * operand = callinst->getOperand(argNo);
						value(operand);
					}
				}
			}
		}
	}

	void functionReturn(llvm::Function * F) {
	#ifdef _DEBUG
		llvm::errs() << "	FunctionReturn" << "\n";
	#endif
		for (llvm::inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
			if (llvm::isa<llvm::ReturnInst>(*I)) {
				llvm::ReturnInst * ret =  llvm::dyn_cast<llvm::ReturnInst>(&*I);
				llvm::Value * retV = ret->getReturnValue();
				value(retV);
			}
		}
	}
	
	void function(llvm::Function * func) {
	#ifdef _DEBUG
		llvm::errs() << "	Function" << "\n";
	#endif
		funcNames.insert(func->getName().str());
	}

	void phiNode(llvm::PHINode * phinode) {
	#ifdef _DEBUG
		llvm::errs() << "	PHINode" << "\n";
	#endif
		int numPhi = phinode->getNumIncomingValues();
		for (int i = 0; i < numPhi; i++) {
			llvm::Value * incomeV = phinode->getIncomingValue(i);
			value(incomeV);
		}
	}

	void callInst(llvm::CallInst * callinst) {
		int lineno = callinst->getDebugLoc().getLine();
		llvm::Function * func = callinst->getCalledFunction();
		if (func) {
			llvm::errs() << lineno << " : " << func->getName().str() << "\n";
		} else {
			funcNames.clear();
			llvm::Value * operand = callinst->getCalledOperand();
			value(operand);
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
