/*
  Copyright 2015 Google LLC All rights reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/*
   american fuzzy lop - LLVM-mode instrumentation pass
   ---------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres. C bits copied-and-pasted
   from afl-as.c are Michal's fault.

   This library is plugged into LLVM when invoking clang through afl-clang-fast.
   It tells the compiler to add code roughly equivalent to the bits discussed
   in ../afl-as.h.
*/

#define AFL_LLVM_PASS

#include "../config.h"
#include "../debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"


#include <map>
#include "llvm/IR/InstVisitor.h"
#include <stack>
#include "llvm/Analysis/CallGraph.h"
#include <set>
#include "llvm/ADT/SCCIterator.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/IR/CFG.h"
using namespace llvm;
using namespace std;

#define UNHANDLETY "Unsupported Type"

namespace {

  class AFLCoverage : public ModulePass {
    public:
      static char ID;
      AFLCoverage() : ModulePass(ID) { }
      bool runOnModule(Module &M) override;
  };

}


char AFLCoverage::ID = 0;


class Visitor : public InstVisitor<Visitor> {
//  vector<BasicBlock *> path;
    set<Value*> states;
    map<Value *, set < Value * >> VMap;
    set<PHINode*> phiNodes;
    set<Value*> ptrChange;
    set<Function*> SCCFunc;

    bool checkState(Value* val){
        if (states.count(val)) return true;
        if (isa<GlobalVariable>(val)) return true;
        return false;
    }


public:
    set<Value*> stateVars;
    map<Value*, Value*> ptrLink;
    set<Value*> infectedPtrSet;
    bool isBranch = false;
    bool isReturn = false;
    bool isRec = false;
    set<Value*> indirectCall;
    set<Function*> directCall;
    set<Value*> getValuesFromStack(Value* v){
        if(isa<Constant>(v)){
            return {};
        }
        auto it = VMap.find(v);
        if (it == VMap.end()){
            if(checkState(v))
                return {v};
            else
                return {};
        }
        set < Value * > related;
        for(auto inst:it->second){
            if(checkState(inst)){
                related.insert(inst);
            }
        }
        return related;
    }
    map<Value *, set < Value *>> getVMap(){
        return VMap;
    }
    explicit Visitor(set<Value*> stateValues, set<Function*> SCCs):
            states(std::move(stateValues)), SCCFunc(std::move(SCCs)){
    }
    void link(BasicBlock * BB){
        isBranch = false;
        isReturn = false;
        isRec = false;
        for (auto &I: *BB){
            visit(&I);
        }
    }
    void parsePHI(){
        for(auto PN: phiNodes){
            set<Value*> incommings;
            for(Value *incom: PN->incoming_values()){
                auto related = getValuesFromStack(incom);
                incommings.insert(related.begin(), related.end());
            }
            VMap.insert(make_pair(PN, incommings));
        }
    }


    //------begin visit------------
    void visitBinaryOperator(BinaryOperator &I){
        unsigned int opNumber = I.getNumOperands();
        set<Value*> ops;
        set<Value*> opins;
        for (unsigned int i = 0; i < opNumber; ++i){
            auto relatedVal = getValuesFromStack(I.getOperand(i));
            ops.insert(relatedVal.begin(), relatedVal.end());
        }
        VMap.insert(make_pair(&I, ops));
    };

    void visitPHINode(PHINode &PN) {
        phiNodes.insert(&PN);
    };

    void visitCallInst(CallInst &I) {
        auto calledFunc = I.getCalledFunction();
        if(!calledFunc){
            indirectCall.insert(I.getCalledOperand());
            return ;
        }
        //        outs()<<"end"<<"visitCallInst"<<'\n';
        if (calledFunc->getName() == StringRef("llvm.dbg.value"))
            return;
        directCall.insert(calledFunc);
        if(SCCFunc.count(I.getFunction())){
            isRec = true;
        }
        set<Value*> funcArgs;
        for(auto ci = I.arg_begin(); ci != I.arg_end(); ++ci){
            auto related = getValuesFromStack(ci->get());
            funcArgs.insert(related.begin(), related.end());
        }
        VMap.insert(make_pair(&I, funcArgs));
        if (SCCFunc.count(calledFunc)){
            stateVars.insert(funcArgs.begin(), funcArgs.end());
        }
    };

    void visitICmpInst(ICmpInst &I){
        //        outs()<<"check"<<"visitICmpInst"<<'\n';
        set<Value*> ops;
        unsigned numOperands = I.getNumOperands();
        for (unsigned i = 0; i < numOperands; ++i) {
            Value *operand = I.getOperand(i);
            set<Value*> subOps = getValuesFromStack(operand);
            for (auto op: subOps){
                ops.insert(op);
            }
        }
        VMap.insert(make_pair(&I, ops));
    };

    void visitBranchInst(BranchInst &I){
        if (I.isUnconditional()) return;
        isBranch = true;
        Value* cmpIns = I.getCondition();
        auto subOps = getValuesFromStack(cmpIns);
        VMap.insert(make_pair(&I, subOps));
    };

    void visitZExtInst(ZExtInst &I){
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    };

    void visitReturnInst(ReturnInst &I) {
        isReturn = true;
    };

    void visitSwitchInst(SwitchInst &I) {
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    };

    void visitIndirectBrInst(IndirectBrInst &I) {
        //Cannot handle yet
    };

    void visitFCmpInst(FCmpInst &I) {
        unsigned int opNumber = I.getNumOperands();
        set<Value*> ops;
        for (unsigned int i = 0; i < opNumber; ++i){
            set<Value*> subOps = getValuesFromStack(I.getOperand(i));
            for (auto op: subOps){
                ops.insert(op);
            }
        }
        VMap.insert(make_pair(&I, ops));
    };

    void visitAllocaInst(AllocaInst &I){
//    VMap.insert(make_pair(&I, set<Value*>({})));
    };

    void visitLoadInst(LoadInst &I){
        auto ptrOp = I.getPointerOperand();
        auto relatedVars = getValuesFromStack(ptrOp);
        VMap.insert(make_pair(&I, relatedVars));
    };
    void visitStoreInst(StoreInst &I){
        auto sourceValue = I.getOperand(0);
        auto targetValue = I.getOperand(1);
        auto iter = VMap.find(targetValue);
        if (iter != VMap.end()){
            VMap.erase(iter);
        }
        auto relatedVars = getValuesFromStack(sourceValue);
        VMap.insert(make_pair(targetValue, relatedVars));
    }

    void visitGetElementPtrInst(GetElementPtrInst &I) {
        auto ptrOp = getValuesFromStack(I.getPointerOperand());
        set<Value*> relatedVals;
        relatedVals.insert(ptrOp.begin(), ptrOp.end());
    }

    void visitTruncInst(TruncInst &I) {
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    }

    void visitSExtInst(SExtInst &I) {
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    }

    void visitFPTruncInst(FPTruncInst &I) {
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    }

    void visitFPExtInst(FPExtInst &I) {
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    }

    void visitUIToFPInst(UIToFPInst &I) {
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    }

    void visitSIToFPInst(SIToFPInst &I) {
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    }

    void visitFPToUIInst(FPToUIInst &I) {
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    }

    void visitFPToSIInst(FPToSIInst &I) {
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    }

    void visitPtrToIntInst(PtrToIntInst &I) {
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    }

    void visitIntToPtrInst(IntToPtrInst &I) {
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    }

    void visitBitCastInst(BitCastInst &I) {
        auto first = getValuesFromStack(I.getOperand(0));
        VMap.insert(make_pair(&I, first));
    }

    void visitSelectInst(SelectInst &I) {
        set<Value*> relatedVals;
        auto relatedCondition = getValuesFromStack(I.getCondition());
        auto relatedTrueValue = getValuesFromStack(I.getTrueValue());
        auto relatedFalseValue = getValuesFromStack(I.getFalseValue());
        relatedVals.insert(relatedCondition.begin(), relatedCondition.end());
        relatedVals.insert(relatedTrueValue.begin(), relatedTrueValue.end());
        relatedVals.insert(relatedFalseValue.begin(), relatedFalseValue.end());

        VMap.insert(make_pair(&I, relatedVals));}

    void visitInvokeInst(InvokeInst &I) {

    }

    void visitUnreachableInst(UnreachableInst &I) {
    }

    void visitShl(BinaryOperator &I) {
        unsigned int opNumber = I.getNumOperands();
        set<Value*> ops;
        set<Value*> opins;
        for (unsigned int i = 0; i < opNumber; ++i){
            auto relatedVal = getValuesFromStack(I.getOperand(i));
            ops.insert(relatedVal.begin(), relatedVal.end());
        }
        VMap.insert(make_pair(&I, ops));
    }

    void visitLShr(BinaryOperator &I) {

    }

    void visitAShr(BinaryOperator &I) {
    }

    void visitVAArgInst(VAArgInst &I) {
//    instMap.insert(make_pair(&I, set<Value*>({I.getPointerOperand()})));
        VMap.insert(make_pair(&I, getValuesFromStack(I.getPointerOperand())));
    }

    void visitExtractElementInst(ExtractElementInst &I) {
//    instMap.insert(make_pair(&I, set<Value*>({I.getOperand(1)})));
        VMap.insert(make_pair(&I, getValuesFromStack(I.getOperand(1))));
    }

    void visitInsertElementInst(InsertElementInst &I) {
        VMap.insert(make_pair(&I, getValuesFromStack(I.getOperand(1))));
    }

    void visitShuffleVectorInst(ShuffleVectorInst &I) {
    }

    void visitExtractValueInst(ExtractValueInst &I) {
//    instMap.insert(make_pair(&I, set<Value*>({I.getAggregateOperand()})));
        VMap.insert(make_pair(&I, getValuesFromStack(I.getAggregateOperand())));
    }

    void visitInsertValueInst(InsertValueInst &I) {

    }

    void visitInstruction(Instruction &I) {
    }
};


class FunctionParser{
    Function* func;
//  vector<vector<BasicBlock*>> Paths;
    vector<CallGraphNode*> fuzzerSCC;
//  vector<vector<BasicBlock*>> retPaths;
//    vector<vector<BasicBlock*>> recurPaths;
    map<Value*, set<Value*>> VMap;
    set<Value*> stateValues;
    set<BasicBlock*> retBlks;
//  map<Value*, Value*> ptrStack;
//  set<Function*> directCalls;
//  set<Function*> indirectCalls;
    static set<BranchInst*> getBranchVals(const set<BasicBlock*>& brBlks, const set<BasicBlock*>& retPathBlks){
        set<BranchInst*> brInsts;
        for (auto blkIt: brBlks){
            bool isOuter = false;
            for(auto succIter = succ_begin(blkIt); succIter != succ_end(blkIt); succIter++){
                if(!retPathBlks.count(*succIter)){
                    isOuter = true;
                    break;
                }
            }
            if(isOuter){
                for(auto instIt = blkIt->begin(); instIt != blkIt->end(); instIt++){
                    if(auto brInst = dyn_cast<BranchInst>(instIt)){
                        brInsts.insert(brInst);
                    }
                }
            }

        }
        return brInsts;
    }
    void getPath(){
        stack<BasicBlock*> BBStack;
        stack<vector<BasicBlock*>> pathStack;
        stack<set<BasicBlock*>> branchStack;
        stack<bool> isRecStack;
        set<BasicBlock*> visitedBrBlks;
        BBStack.push(&(func->getEntryBlock()));
        pathStack.push(vector<BasicBlock*> {&func->getEntryBlock()});
        isRecStack.push(false);
        branchStack.push(set<BasicBlock*> {});
        set<Value*> funcArgs;
        set<Function*> SCCFuncs;
        set<BasicBlock*> retPathBlks;
        for(auto CGN: fuzzerSCC){
            SCCFuncs.insert(CGN->getFunction());
        }
        for(auto arg = func->arg_begin(); arg != func->arg_end(); arg++){
            funcArgs.insert(arg);
        }
        Visitor BBVisitor(funcArgs, SCCFuncs);
        while(!BBStack.empty()){
            BasicBlock* CurBB =  BBStack.top();
            BBStack.pop();
//            CurBB->dump();
//            errs()<<CurBB<<'\n';
            vector<BasicBlock*> curPath = pathStack.top();
            pathStack.pop();
            bool isRec = isRecStack.top();
            isRecStack.pop();
            set<BasicBlock*> visitedBranchs = branchStack.top();
            branchStack.pop();
            BBVisitor.link(CurBB);
            isRec = isRec or BBVisitor.isRec;
//      if(BBVisitor)
            isRecStack.push(isRec);

            if(BBVisitor.isBranch){
                visitedBranchs.insert(CurBB);
            }
            branchStack.push(visitedBranchs);
            if (BBVisitor.isReturn)
                retBlks.insert(CurBB);
            if(BBVisitor.isReturn && !isRec){
//        retPaths.push_back(curPath);
                retPathBlks.insert(curPath.begin(), curPath.end());
                visitedBrBlks.insert(visitedBranchs.begin(), visitedBranchs.end());
            }
//            errs()<<"checkPoint2"<<'\n';

//            if (succ_begin(CurBB) == succ_end(CurBB)){
//                recurPaths.push_back(curPath);
//            }
            for (succ_iterator succPtr = succ_begin(CurBB); succPtr != succ_end(CurBB); succPtr++){
                BasicBlock* sucBB = *succPtr;
//                errs()<<"checkPoint3"<<'\n';
                if (std::find(curPath.begin(), curPath.end(), sucBB) == curPath.end()){
                    vector<BasicBlock *> newPath = curPath;
                    newPath.push_back(sucBB);
                    BBStack.push(sucBB);
                    pathStack.push(newPath);
                }
            }
        }
        BBVisitor.parsePHI();
        auto brInsts = getBranchVals(visitedBrBlks, retPathBlks);

        for (auto br: brInsts){
            auto vals = BBVisitor.getValuesFromStack(br);
            stateValues.insert(vals.begin(), vals.end());
        }
        stateValues.insert(BBVisitor.stateVars.begin(), BBVisitor.stateVars.end());
    }

public:
    set<BasicBlock*> getReturnBlk(){
        return retBlks;
    }
    set<Value*> getStates(){
        return stateValues;
    }
    void run(){
        getPath();
    }
    explicit FunctionParser(Function* targetFunc, vector<CallGraphNode*> SCC):func(targetFunc), fuzzerSCC(std::move(SCC)){}
};



void SCCPruning(map<CallGraphNode*, set<CallGraphNode*>>& preNodes, CallGraphNode* pivot,
                set<CallGraphNode*>& candidate, map<CallGraphNode*, int> &outDegree){

    //  candidate.erase(pivot);
    //  outDegree.erase(pivot);
    auto preNodeIt = preNodes.find(pivot);
    if (preNodeIt != preNodes.end()){
        auto allPreNodes = preNodeIt->second;
        for (auto preIt:allPreNodes){
            if (candidate.find(preIt) != candidate.end()){
                outDegree[preIt]--;
                if (outDegree[preIt] <= 0){
                    candidate.erase(preIt);
                    outDegree.erase(preIt);
                    SCCPruning(preNodes, preIt,
                               candidate, outDegree);
                }
            }
        }
    }
}

class SCCParser{
    vector<CallGraphNode *> SCC;
    set<BasicBlock*> returnBlocks;
    map<CallGraphNode*, set<CallGraphNode*>> preNodes;
    vector<Value*> globalVariables;
public:
    set<CallGraphNode *> pivotNodes;
    explicit SCCParser(vector<CallGraphNode *> scc):SCC(std::move(scc)){
    }
    void link(){
        set<CallGraphNode *> candidateNodes;
        map<CallGraphNode*, int> outDegree;
        int maxDegree = 0;
        CallGraphNode * pivot;

        for (CallGraphNode *Node : SCC) {
            int currentDegree = 0;
            set<CallGraphNode*> visitedCG;
            for(auto & CI : *Node){
                auto target = CI.second;

//                errs()<<"node edge:"<<Node->getFunction()->getName()<<"->"<<CI.second->getFunction()->getName()<<'\n';
                if(std::find(SCC.begin(), SCC.end(), target) != SCC.end() and !visitedCG.count(target)){
                    visitedCG.insert(target);
                    currentDegree++;
                    auto it = preNodes.find(target);
                    if (it != preNodes.end()){
                        it->second.insert(Node);
                    }else{
                        set <CallGraphNode*> pre = {Node};
                        preNodes.insert(make_pair(target, pre));
                    }
                }
            }
            outDegree.insert(make_pair(Node, currentDegree));
            if(currentDegree > maxDegree){
                maxDegree = currentDegree;
                pivot = Node;
            }
            candidateNodes.insert(Node);
        }
        if (pivot)
            pivotNodes.insert(pivot);
        bool isSCC = true;
        while (isSCC){
            errs()<<"pivot:";
//            pivot->dump();
            candidateNodes.erase(pivot);
            outDegree.erase(pivot);
            if(candidateNodes.empty()) break;
            SCCPruning(preNodes, pivot,
                       candidateNodes, outDegree);
            isSCC = false;
            maxDegree = 0;
            for (auto & degreeIt : outDegree){
                if (degreeIt.second > 0){
                    isSCC = true;
                    if (degreeIt.second > maxDegree){
                        pivot = degreeIt.first;
                    }
                }
            }
            pivotNodes.insert(pivot);
        }
    }
};



Function* getRequiredHash(Value* val, vector<Function*> hashFuncs){
    Type* valType = val->getType();
    switch (valType->getTypeID()) {
        case Type::FloatTyID:
            return hashFuncs[0];
        case Type::DoubleTyID:
            return hashFuncs[1];
        case Type::X86_FP80TyID:
            return hashFuncs[2];
        case Type::IntegerTyID:{
            auto bitSize = valType->getScalarSizeInBits();
            switch (bitSize) {
                case 1:
                    return hashFuncs[3];
                case 8:
                    return hashFuncs[4];
                case 16:
                    return hashFuncs[5];
                case 32:
                    return hashFuncs[6];
                case 64:
                    return hashFuncs[7];
                default:
                    break;
            }
        }

        case Type::PointerTyID:
            return hashFuncs[8];

        default:
            return nullptr;
    }
}

bool AFLCoverage::runOnModule(Module &M) {
    unsigned long recordNum = 1000;

    LLVMContext &C = M.getContext();
    IRBuilder<> ModuleBuilder(C);
    IntegerType *Int8Ty  = IntegerType::getInt8Ty(C);
    IntegerType *Int32Ty = IntegerType::getInt32Ty(C);
    llvm::CallGraph CG = llvm::CallGraph(M);
//  for (auto &fc : M.getFunctionList()){
//      errs()<<"print functions:" << fc.getName()<<'\n';
//  }
//    CallGraph &CG = MAM.getResult<CallGraphAnalysis>(M);
//  vector<CallGraphNode*> pivotNodes;
    errs()<<"begin recursion analysis!"<<'\n';
//    outs()<<"the number of SCC:"<< count(&CG)
    IntegerType *Int1Ty = IntegerType::getInt1Ty(C);
    IntegerType *Int16Ty = IntegerType::getInt16Ty(C);
    IntegerType *Int64Ty = IntegerType::getInt64Ty(C);
    PointerType *Int8PtrTy = PointerType::getInt8PtrTy(C);
    PointerType *Int8PtrPtrTy = PointerType::get(Int8PtrTy, 0);
    PointerType *Int64PtrTy = PointerType::getInt64PtrTy(C);
    PointerType *Int32PtrTy = PointerType::getInt32PtrTy(C);
    Type* VoidTy = Type::getVoidTy(C);

    Function* floatHash = Function::Create(FunctionType::get(Int64Ty, ArrayRef<Type*>({Int8Ty}), true), GlobalValue::ExternalLinkage, "floatHash", M);
     Function* doubleHash = Function::Create(FunctionType::get(Int64Ty, ArrayRef<Type*>({Int32Ty}), true), GlobalValue::ExternalLinkage, "doubleHash", M);
     Function* longDoubleHash = Function::Create(FunctionType::get(Int64Ty, ArrayRef<Type*>({Int64Ty}), true), GlobalValue::ExternalLinkage, "longDoubleHash", M);
     Function* boolHash = Function::Create(FunctionType::get(Int64Ty, ArrayRef<Type*>({Int1Ty}), true), GlobalValue::ExternalLinkage, "boolHash", M);
     Function* charHash = Function::Create(FunctionType::get(Int64Ty, ArrayRef<Type*>({Int8Ty}), true), GlobalValue::ExternalLinkage, "charHash", M);
     Function* i16Hash = Function::Create(FunctionType::get(Int64Ty, ArrayRef<Type*>({Int16Ty}), true), GlobalValue::ExternalLinkage, "i16Hash", M);
     Function* i32Hash = Function::Create(FunctionType::get(Int64Ty, ArrayRef<Type*>({Int32Ty}), true), GlobalValue::ExternalLinkage, "i32Hash", M);
     Function* i64Hash = Function::Create(FunctionType::get(Int64Ty, ArrayRef<Type*>({Int64Ty}), true), GlobalValue::ExternalLinkage, "i64Hash", M);
     Function* ptrHash = Function::Create(FunctionType::get(Int64Ty, ArrayRef<Type*>({Int64Ty}), true), GlobalValue::ExternalLinkage, "ptrHash", M);
     Function* hashCombineFunc = Function::Create(FunctionType::get(Int8PtrTy, ArrayRef<Type*>({Int64PtrTy, Int32Ty}), true), GlobalValue::ExternalLinkage, "_hashCombine", M);
     Function* checkFunc = Function::Create(FunctionType::get(VoidTy, ArrayRef<Type*>({Int8PtrPtrTy, Int8PtrTy, Int32PtrTy, Int32PtrTy}), true), GlobalValue::ExternalLinkage, "_entryCheck", M);
     Function* checkReturn = Function::Create(FunctionType::get(VoidTy, ArrayRef<Type*>({Int8PtrPtrTy, Int8PtrTy, Int32PtrTy, Int32PtrTy}), true), GlobalValue::ExternalLinkage, "_checkReturn", M);



    auto recordArrayTy = ArrayType::get(Int8PtrTy, recordNum);
    Constant *arrayInitializer = ConstantAggregateZero::get(recordArrayTy);
//    unsigned int recNum = 0;
    for (scc_iterator < CallGraph * > I = scc_begin(&CG); !I.isAtEnd();
         ++I) {

        if(!I.hasLoop()) continue;
        set<CallGraphNode *> candidateNodes;
        const std::vector<CallGraphNode *> &SCC = *I;
        errs() << "found SCC!" << '\n';
        for(auto ND: SCC){
            errs()<<"print functions:" << ND->getFunction()->getName()<<'\n';
        }

        SCCParser findPivot(SCC);
        findPivot.link();
        errs() << "findPivot!" << '\n';
        auto pivots = findPivot.pivotNodes;
        for (auto CGN: pivots) {
            Function* targetFunc = CGN->getFunction();
            ///build a global counter for each funciton
            Constant *initializer = ConstantInt::get(Int32Ty, 0);
            string counterName = "_counter_" + targetFunc->getName().str();
            string stackIterName = "_sIter_" + targetFunc->getName().str();
            GlobalVariable *globalCounter =
                    new GlobalVariable(M, Type::getInt32Ty(C), false,
                                       GlobalValue::ExternalLinkage, initializer, counterName);
            GlobalVariable *globalStackIter =
                    new GlobalVariable(M, Type::getInt32Ty(C), false,
                                       GlobalValue::ExternalLinkage, initializer, stackIterName);

            ///global array
            string recordArrayName = "_recordArray_" + targetFunc->getName().str();
            GlobalVariable *newGlobalArray =
                    new GlobalVariable(M, recordArrayTy, false,
                                       GlobalValue::ExternalLinkage, arrayInitializer, recordArrayName);


            FunctionParser funcParser(targetFunc, SCC);
            errs() << "funcParser!" << '\n';
            funcParser.run();
            errs() << "checkpoint1" << '\n';
            auto retBlks =funcParser.getReturnBlk();
            auto states = funcParser.getStates();
            auto entryIP = targetFunc->getEntryBlock().getFirstInsertionPt();


            IRBuilder<> entryIRB(&(*entryIP));
            ///instrumented number
//            auto instrumentNum = entryIRB.CreateAlloca(Int32Ty);
//            entryIRB.CreateStore(ConstantInt::get(Int32Ty, 0), instrumentNum)->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

            ///the args to be hashed
            string labelNames = "_label_"+ targetFunc->getName().str();
            ArrayType* arrayType = ArrayType::get(Int64Ty, states.size());
            auto labelArray = entryIRB.CreateAlloca(arrayType, nullptr, labelNames);
//            auto counter = entryIRB.CreateAlloca(Int32Ty, nullptr, counterName);
//            entryIRB.CreateStore(ConstantInt::get(Int32Ty, 0), counter)->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

//            auto recordArray = entryIRB.CreateAlloca(recordArrayTy,nullptr, recordArrayName);
            int stateIter = 0;
            for (auto s: states) {
                auto func = getRequiredHash(s, {floatHash, doubleHash, longDoubleHash, boolHash, charHash, i16Hash, i32Hash, i64Hash, ptrHash});
                assert(func != nullptr&& UNHANDLETY);
                Value* parameter;
                if (s->getType()->getTypeID() == Type::PointerTyID){
                    auto para = entryIRB.CreatePtrToInt(s, entryIRB.getInt64Ty());
                    parameter = entryIRB.CreateZExt(para, entryIRB.getInt64Ty());
                } else
                    parameter = s;
                auto getHash = entryIRB.CreateCall(func, ArrayRef<Value*>({parameter}));
                auto ind = ConstantInt::get(Int32Ty, stateIter);
                stateIter++;
                auto ptr = entryIRB.CreateGEP(labelArray, {ConstantInt::get(Int32Ty, 0), ind});
                entryIRB.CreateStore(getHash, ptr);

            }
            unsigned int stateNum = states.size();
            auto labelArrPtr = entryIRB.CreateGEP(labelArray, {ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0)});
            auto newVal = entryIRB.CreateCall(hashCombineFunc, ArrayRef<Value*>({labelArrPtr, ConstantInt::get(Int32Ty, stateNum)}));
            auto oldArrPtr = entryIRB.CreateGEP(newGlobalArray, {ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0)});
//            errs()<<"bad1:"<<'\n';
            entryIRB.CreateCall(checkFunc, ArrayRef<Value*>({oldArrPtr, newVal, globalCounter, globalStackIter}))->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
//            errs()<<"bad"<<"1-1:"<<'\n';
            int n = 0;
            for(auto bbIt: retBlks){
                n++;
                auto firstInst = bbIt->getFirstInsertionPt();
                IRBuilder<> retBuilder(&(*firstInst));
                errs()<<"bad"<<n<<':'<<'\n';
//                auto oldArrPtr2 = retBuilder.CreateGEP(newGlobalArray, {ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0)});
                retBuilder.CreateCall(checkReturn, ArrayRef<Value*>({oldArrPtr, newVal, globalCounter, globalStackIter}))->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
            }

//            errs() << "function:" << CGN->getFunction()->getName() << '\n';

        }
    }
















  /* The following is AFL part*/
  /* Show a banner */

  char be_quiet = 0;

  if (isatty(2) && !getenv("AFL_QUIET")) {

    SAYF(cCYA "afl-llvm-pass " cBRI VERSION cRST " by <lszekeres@google.com>\n");

  } else be_quiet = 1;

  /* Decide instrumentation ratio */

  char* inst_ratio_str = getenv("AFL_INST_RATIO");
  unsigned int inst_ratio = 100;

  if (inst_ratio_str) {

    if (sscanf(inst_ratio_str, "%u", &inst_ratio) != 1 || !inst_ratio ||
        inst_ratio > 100)
      FATAL("Bad value of AFL_INST_RATIO (must be between 1 and 100)");

  }

  /* Get globals for the SHM region and the previous location. Note that
     __afl_prev_loc is thread-local. */

  GlobalVariable *AFLMapPtr =
      new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                         GlobalValue::ExternalLinkage, 0, "__afl_area_ptr");

  GlobalVariable *AFLPrevLoc = new GlobalVariable(
      M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__afl_prev_loc",
      0, GlobalVariable::GeneralDynamicTLSModel, 0, false);

  /* Instrument all the things! */

  int inst_blocks = 0;

  for (auto &F : M)
    for (auto &BB : F) {

      BasicBlock::iterator IP = BB.getFirstInsertionPt();
      IRBuilder<> IRB(&(*IP));

      if (AFL_R(100) >= inst_ratio) continue;

      /* Make up cur_loc */

      unsigned int cur_loc = AFL_R(MAP_SIZE);

      ConstantInt *CurLoc = ConstantInt::get(Int32Ty, cur_loc);

      /* Load prev_loc */

      LoadInst *PrevLoc = IRB.CreateLoad(AFLPrevLoc);
      PrevLoc->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      Value *PrevLocCasted = IRB.CreateZExt(PrevLoc, IRB.getInt32Ty());

      /* Load SHM pointer */

      LoadInst *MapPtr = IRB.CreateLoad(AFLMapPtr);
      MapPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      Value *MapPtrIdx =
          IRB.CreateGEP(MapPtr, IRB.CreateXor(PrevLocCasted, CurLoc));

      /* Update bitmap */

      LoadInst *Counter = IRB.CreateLoad(MapPtrIdx);
      Counter->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      Value *Incr = IRB.CreateAdd(Counter, ConstantInt::get(Int8Ty, 1));
      IRB.CreateStore(Incr, MapPtrIdx)
          ->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

      /* Set prev_loc to cur_loc >> 1 */

      StoreInst *Store =
          IRB.CreateStore(ConstantInt::get(Int32Ty, cur_loc >> 1), AFLPrevLoc);
      Store->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

      inst_blocks++;

    }

  /* Say something nice. */

  if (!be_quiet) {

    if (!inst_blocks) WARNF("No instrumentation targets found.");
    else OKF("Instrumented %u locations (%s mode, ratio %u%%).",
             inst_blocks, getenv("AFL_HARDEN") ? "hardened" :
             ((getenv("AFL_USE_ASAN") || getenv("AFL_USE_MSAN")) ?
              "ASAN/MSAN" : "non-hardened"), inst_ratio);

  }

  return true;

}


static void registerAFLPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {

  PM.add(new AFLCoverage());

}


static RegisterStandardPasses RegisterAFLPass(
    PassManagerBuilder::EP_ModuleOptimizerEarly, registerAFLPass);

static RegisterStandardPasses RegisterAFLPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerAFLPass);
