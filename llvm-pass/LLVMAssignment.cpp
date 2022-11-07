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
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>

#define _DEBUG

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
	FuncPtrPass() : llvm::ModulePass(ID) {}
	
	void function (llvm::Function * func) {
		llvm::outs() << func->getName().data();
	}

	void phiNode (llvm::PHINode * phinode) {
		int numPhi = phinode->getNumIncomingValues();
		for (int i = 0; i < numPhi; i++) {
			llvm::Value * incomeV = phinode->getIncomingValue(i);
			if (llvm::isa<llvm::CallInst>(incomeV)) {
				callInst(llvm::dyn_cast<llvm::CallInst>(incomeV));
			} else if (llvm::isa<llvm::Function>(incomeV)) {
				function(llvm::dyn_cast<llvm::Function>(incomeV));
			}
		}
	}

	void callInst(llvm::CallInst * callinst) {
		int lineno = callinst->getDebugLoc().getLine();
		llvm::Function * func = callinst->getCalledFunction();
		if (func) {
			llvm::outs() << lineno << " : " << func->getName().data() << "\n";
		} else {
			llvm::outs() << lineno << " : ";
			llvm::Value * operand = callinst->getCalledOperand();
			if (llvm::isa<llvm::PHINode>(operand)) {
				phiNode(llvm::dyn_cast<llvm::PHINode>(operand));
			}
			llvm::outs() << "\n";
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
		for (llvm::Module::iterator fi = M.begin(), fe = M.end(); fi != fe; fi++) {
			llvm::Function &f = *fi;
			for (llvm::Function::iterator bi = f.begin(), be = f.end(); bi != be; bi++) {
				llvm::BasicBlock &b = *bi;
				for (llvm::BasicBlock::iterator ii = b.begin(), ie = b.end(); ii != ie; ii++) {
					llvm::Instruction &i = *ii;
					//需要排除llvm.dbg.value
					if (llvm::isa<llvm::CallInst>(i) && !llvm::isa<llvm::DbgInfoIntrinsic>(i)) {
						callInst(llvm::dyn_cast<llvm::CallInst>(&i));
					}
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
