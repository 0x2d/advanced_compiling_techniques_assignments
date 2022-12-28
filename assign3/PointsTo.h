#include <llvm/IR/Function.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/IntrinsicInst.h>

#include <set>
#include <map>
#include <queue>

#include "Dataflow.h"
using namespace llvm;

// #define _DEBUG

struct PointsToInfo {
    std::map<Value *, std::set<Value *>> pointsToSets;
    std::map<Value *, std::set<Value *>> aliases;

    bool operator== (const PointsToInfo &info) const {
        return pointsToSets == info.pointsToSets && aliases == info.aliases;
    }
};

inline raw_ostream &operator<< (raw_ostream &out, const PointsToInfo &info) {
    out << "\nPoints-to Sets: \n";
    for (auto pts: info.pointsToSets) {
        if (pts.first->hasName()) {
            out << pts.first->getName() << " : ";
        } else {
            pts.first->print(out);
            out << " : ";
        }
        for (auto p: pts.second) {
            if (p == NULL) {
                continue;
            }
            if (p->hasName()) {
                out << p->getName() << ", ";
            } else {
                p->print(out);
                out << ", ";
            }
        }
        out << "\n";
    }

    out << "\nAliases: \n";
    for (auto pts: info.aliases) {
        if (pts.first->hasName()) {
            out << pts.first->getName() << " : ";
        } else {
            pts.first->print(out);
            out << " : ";
        }
        for (auto p: pts.second) {
            if (p == NULL) {
                continue;
            }
            if (p->hasName()) {
                out << p->getName() << ", ";
            } else {
                p->print(out);
                out << ", ";
            }
        }
        out << "\n";
    }
    return out;
}

class PointsToVisitor : public DataflowVisitor<struct PointsToInfo> {
public:
    std::map<int, std::set<std::string>> results;

    PointsToVisitor() {}

    void merge(PointsToInfo * dest, const PointsToInfo & src) override {
        for (auto &pts: src.pointsToSets) {
            auto tempfind = dest->pointsToSets.find(pts.first);
            if (tempfind == dest->pointsToSets.end()) {
                dest->pointsToSets.insert(pts);
            } else {
                tempfind->second.insert(pts.second.begin(), pts.second.end());
            }
        }

        for (auto &pts: src.aliases) {
            auto tempfind = dest->aliases.find(pts.first);
            if (tempfind == dest->aliases.end()) {
                dest->aliases.insert(pts);
            } else {
                tempfind->second.insert(pts.second.begin(), pts.second.end());
            }
        }
    }

    void compDFVal(Instruction *inst, PointsToInfo * dfval) override {
        if (isa<DbgInfoIntrinsic>(inst)) {
            return;
        }

        if (isa<StoreInst>(inst)) {
            StoreInst * storeinst = dyn_cast<StoreInst>(inst);
            Value * value = storeinst->getValueOperand();
            Value * pointer = storeinst->getPointerOperand();

            if (isa<ConstantData>(value)) {
                return;
            }

            //令pointer，要么指向value，要么指向value的别名；如果pointer是某个指针的别名，则递归地找到pointer的核心的别名，替换其指向集为values
            std::set<Value *> values = {};
            if (dfval->aliases.find(value) != dfval->aliases.end()) {
                values.insert(dfval->aliases[value].begin(), dfval->aliases[value].end());
            } else {
                values.insert(value);
            }
            std::queue<Value *> worklist;
            worklist.push(pointer);
            std::set<Value *> alias = {};
            while (worklist.size() > 0) {
                Value * temp = worklist.front();
                worklist.pop();
                if (dfval->aliases.find(temp) != dfval->aliases.end()) {
                    for (auto ap: dfval->aliases[temp]) {
                        worklist.push(ap);
                    }
                } else {
                    alias.insert(temp);
                }
            }
            //当有多个别名时，不能替换，而是应该合并
            if (alias.size() == 1) {
                for (auto p: alias) {
                    dfval->pointsToSets[p] = values;
                }
            } else {
                for (auto p: alias) {
                    dfval->pointsToSets[p].insert(values.begin(), values.end());
                }
            }
            
        } else if (isa<LoadInst>(inst)) {
            LoadInst * loadinst = dyn_cast<LoadInst>(inst);
            Value * pointer = loadinst->getPointerOperand();
            Value * result = loadinst;

            if (!pointer->getType()->getContainedType(0)->isPointerTy()) {
                return;
            }

            //令result的指向集与pointer的指向集的指向集相同，同时设result为pointer指向集的别名
            std::set<Value *> pts = dfval->pointsToSets[pointer];
            dfval->pointsToSets[result] = {};
            dfval->aliases[result] = pts;
            for (auto ap: pts) {
                dfval->pointsToSets[result].insert(dfval->pointsToSets[ap].begin(), dfval->pointsToSets[ap].end());
            }
            if (dfval->pointsToSets[result].size() == 0) {
                dfval->pointsToSets.erase(result);
            }
        } else if (isa<MemSetInst>(inst)) {
            return;
        } else if (isa<MemCpyInst>(inst)) {
            MemCpyInst * memcpyinst = dyn_cast<MemCpyInst>(inst);
            Value *source = memcpyinst->getSource();
            Value *dest = memcpyinst->getDest();

            dfval->pointsToSets[dest] = dfval->pointsToSets[source];
        } else if (isa<CallInst>(inst)) {
            CallInst * callinst = dyn_cast<CallInst>(inst);
            Value * called = callinst->getCalledOperand();
            unsigned lineno = callinst->getDebugLoc().getLine();
            auto & funcNames = results[lineno];
            if (isa<Function>(called) && called->getName() == "malloc") {
                funcNames.insert("malloc");
                return;
            }
            
            std::set<Function *> functions;
            if (isa<Function>(called)) {
                functions.insert(dyn_cast<Function>(called));
            } else {
                auto alias = dfval->aliases[called];
                for (auto f : alias) {
                    functions.insert(dyn_cast<Function>(f));
                }
            }
            
            for (auto f : functions) {
                if (f == NULL) {
                    continue;
                }
                // errs() << *dfval << "\n";
                funcNames.insert(f->getName());
                PointsToInfo calleeInfo = *dfval;
                std::map<Value *, Value *> args;
                for (unsigned i = 0; i < callinst->getNumArgOperands(); i++) {
                    Value * callerArg = callinst->getArgOperand(i);
                    if (callerArg->getType()->isPointerTy()) {
                        Value * calleeArg = f->getArg(i);
                        args[callerArg] = calleeArg;
                        if (dfval->pointsToSets.find(callerArg) != dfval->pointsToSets.end()) {
                            calleeInfo.pointsToSets[calleeArg] = dfval->pointsToSets[callerArg];
                        }
                        if (dfval->aliases.find(callerArg) != dfval->aliases.end()) {
                            calleeInfo.aliases[calleeArg] = dfval->aliases[callerArg];
                        }
                    }
                }
                if (f->getReturnType()->isPointerTy()) {
                    args[callinst] = f;
                }
                PointsToInfo initval;
                PointsToVisitor visitor;
                BasicBlock *targetEntry = &(f->getEntryBlock());
                BasicBlock *targetExit = &(f->back());
                DataflowResult<PointsToInfo>::Type result;
                result[targetEntry].first = calleeInfo;
                compForwardDataflow(f, &visitor, &result, initval);
            #ifdef _DEBUG
                // printDataflowResult<PointsToInfo>(errs(), result);
            #endif
                PointsToInfo calleeResult = result[targetExit].second;
                merge(dfval, calleeResult);
                for (auto p: args) {
                    if (calleeResult.pointsToSets.find(p.second) != calleeResult.pointsToSets.end()) {
                        /*
                        if (dfval->pointsToSets.find(p.first) != dfval->pointsToSets.end()) {
                            dfval->pointsToSets[p.first].insert(calleeResult.pointsToSets[p.second].begin(), calleeResult.pointsToSets[p.second].end());
                        } else {
                            dfval->pointsToSets[p.first] = calleeResult.pointsToSets[p.second];
                        }
                        */
                       //函数的指针参数的指向集不能合并，需要替换
                        dfval->pointsToSets[p.first] = calleeResult.pointsToSets[p.second];
                    }
                    if (calleeResult.aliases.find(p.second) != calleeResult.aliases.end()) {
                        if (dfval->aliases.find(p.first) != dfval->aliases.end()) {
                            dfval->aliases[p.first].insert(calleeResult.aliases[p.second].begin(), calleeResult.aliases[p.second].end());
                        } else {
                            dfval->aliases[p.first] = calleeResult.aliases[p.second];
                        }
                    }
                }

                for (auto line: visitor.results) {
                    if (results.find(line.first) == results.end()) {
                        results[line.first] = line.second;
                    } else {
                        results[line.first].insert(line.second.begin(), line.second.end());
                    }
                }
            }

        } else if (isa<ReturnInst>(inst)) {
            ReturnInst * returninst = dyn_cast<ReturnInst>(inst);
            Value * value = returninst->getReturnValue();
            Value * f = returninst->getFunction();
            //返回值应设置别名而非指向集
            if (dfval->aliases.find(value) != dfval->aliases.end()) {
                dfval->aliases[f] = dfval->aliases[value];
            } else {
                dfval->aliases[f] = {value};
            }
        } else if (isa<GetElementPtrInst>(inst)) {
            GetElementPtrInst * getelementptrinst = dyn_cast<GetElementPtrInst>(inst);
            Value * pointer = getelementptrinst->getPointerOperand();
            Value * result = getelementptrinst;

            //将result设为pointer的别名，同时同步这两个指针的指向集
            dfval->aliases[result] = {pointer};
            if (dfval->pointsToSets.find(pointer) != dfval->pointsToSets.end()) {
                dfval->pointsToSets[result] = dfval->pointsToSets[pointer];
            }
        }
    }

    void print(raw_ostream &out) const {
        for (auto line: results) {
            out << line.first << " : ";
            int index = 0;
            for (auto func: line.second) {
                if (index == 0) {
                    out << func;
                } else {
                    out << ", " << func;
                }
                index++;
            }
            out << "\n";
        }
    }
};

class PointsTo : public ModulePass {
public:
    static char ID;
    PointsTo() : ModulePass(ID) {} 

    bool runOnModule(Module &M) override {
    #ifdef _DEBUG
        M.print(llvm::errs(), nullptr);
	    llvm::errs()<<"------------------------------\n";
    #endif
        PointsToVisitor visitor;
        DataflowResult<PointsToInfo>::Type result;
        PointsToInfo initval;

        auto F = M.rbegin();
        while (F->isIntrinsic() || F->getName() == "malloc") {
            F++;
        }
    #ifdef _DEBUG
        llvm::errs() << "Analyse function : " << F->getName() << "\n";
    #endif
        compForwardDataflow(&*F, &visitor, &result, initval);
    #ifdef _DEBUG
        printDataflowResult<PointsToInfo>(errs(), result);
    #endif
        visitor.print(errs());
        return false;
    }
};
