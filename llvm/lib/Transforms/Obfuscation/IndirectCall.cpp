#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Obfuscation/IndirectCall.h"
#include "llvm/Transforms/Obfuscation/ObfuscationOptions.h"
#include "llvm/Transforms/Obfuscation/IPObfuscationContext.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/Transforms/Obfuscation/CryptoUtils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/AbstractCallSite.h"

#include <random>

#define DEBUG_TYPE "icall"

using namespace llvm;

void IndirectCall::NumberCallees(Function &F) {
  for (auto &BB:F) {
    for (auto &I:BB) {
      if (isa<CallInst>(&I)) {
        auto *CI = cast<CallInst>(&I);
        Function *Callee = CI->getCalledFunction();
        if (Callee == nullptr) {
          continue;
        }
        if (Callee->isIntrinsic()) {
          continue;
        }
        CallSites.push_back(CI);
        if (CalleeNumbering.count(Callee) == 0) {
          CalleeNumbering[Callee] = Callees.size();
          Callees.push_back(Callee);
        }
      }
    }
  }
}
GlobalVariable *IndirectCall::getIndirectCallees(Function &F,
                                                       ConstantInt *EncKey) {
  std::string GVName(F.getName().str() + "_IndirectCallees");
  GlobalVariable *GV = F.getParent()->getNamedGlobal(GVName);
  if (GV)
    return GV;

  // callee's address
  std::vector<Constant *> Elements;
  for (auto Callee:Callees) {
    Constant *CE = ConstantExpr::getBitCast(Callee, Type::getInt8PtrTy(F.getContext()));
    CE = ConstantExpr::getGetElementPtr(Type::getInt8Ty(F.getContext()), CE, EncKey);
    Elements.push_back(CE);
  }

  ArrayType *ATy = ArrayType::get(Type::getInt8PtrTy(F.getContext()), Elements.size());
  Constant *CA = ConstantArray::get(ATy, ArrayRef<Constant *>(Elements));
  GV = new GlobalVariable(*F.getParent(), ATy, false, GlobalValue::LinkageTypes::PrivateLinkage,
                          CA, GVName);
  appendToCompilerUsed(*F.getParent(), {GV});
  return GV;
}

PreservedAnalyses IndirectCall::run(Function &F, FunctionAnalysisManager &AM) {
  if (!toObfuscate(enable, &F, "icall")) {
    return PreservedAnalyses::all();
  }

  if (Options && Options->skipFunction(F.getName())) {
    return PreservedAnalyses::all();
  }

  LLVMContext &Ctx = F.getContext();

  CalleeNumbering.clear();
  Callees.clear();
  CallSites.clear();

  NumberCallees(F);

  if (Callees.empty()) {
    return PreservedAnalyses::all();
  }

  uint32_t V = IPO->RandomEngine.get_uint32_t() & ~3;
  ConstantInt *EncKey = ConstantInt::get(Type::getInt32Ty(Ctx), V, false);

  const IPObfuscationContext::IPOInfo *SecretInfo = nullptr;
  if (IPO) {
    SecretInfo = IPO->getIPOInfo(&F);
  }

  Value *MySecret;
  if (SecretInfo) {
    MySecret = SecretInfo->SecretLI;
  } else {
    MySecret = ConstantInt::get(Type::getInt32Ty(Ctx), 0, true);
  }

  ConstantInt *Zero = ConstantInt::get(Type::getInt32Ty(Ctx), 0);
  GlobalVariable *Targets = getIndirectCallees(F, EncKey);

  for (auto *CI : CallSites) {
    SmallVector<Value *, 8> Args;
    SmallVector<AttributeSet, 8> ArgAttrVec;

    Instruction *Call = CI;
    Function *Callee = CI->getCalledFunction();
    FunctionType *FTy = CI->getFunctionType();
    IRBuilder<> IRB(Call);

    Args.clear();
    ArgAttrVec.clear();

    Value *Idx = ConstantInt::get(Type::getInt32Ty(Ctx), CalleeNumbering[CI->getCalledFunction()]);
    Value *GEP = IRB.CreateGEP(Targets->getType()->getScalarType()->getPointerElementType(),Targets, {Zero, Idx});
    LoadInst *EncDestAddr = IRB.CreateLoad(GEP->getType()->getPointerElementType(),GEP, CI->getName());
    Constant *X;
    if (SecretInfo) {
      X = ConstantExpr::getSub(SecretInfo->SecretCI, EncKey);
    } else {
      X = ConstantExpr::getSub(Zero, EncKey);
    }

    const AttributeList &CallPAL = CI->getAttributes();
    auto* I = CI->arg_begin();
    unsigned i = 0;

    for (unsigned e = FTy->getNumParams(); i != e; ++I, ++i) {
      Args.push_back(*I);
      AttributeSet Attrs = CallPAL.getParamAttrs(i);
      ArgAttrVec.push_back(Attrs);
    }

    for (auto* E = CI->arg_end(); I != E; ++I, ++i) {
      Args.push_back(*I);
      ArgAttrVec.push_back(CallPAL.getParamAttrs(i));
    }

    AttributeList NewCallPAL = AttributeList::get(
        IRB.getContext(), CallPAL.getFnAttrs(), CallPAL.getRetAttrs(), ArgAttrVec);

    Value *Secret = IRB.CreateSub(X, MySecret);
    Value *DestAddr = IRB.CreateGEP(EncDestAddr->getType()->getScalarType()->getPointerElementType(),EncDestAddr, Secret);

    Value *FnPtr = IRB.CreateBitCast(DestAddr, FTy->getPointerTo());
    FnPtr->setName("Call_" + Callee->getName());
    CallInst *NewCall = IRB.CreateCall(FTy, FnPtr, Args, Call->getName());
    NewCall->setAttributes(NewCallPAL);
    Call->replaceAllUsesWith(NewCall);
    Call->eraseFromParent();
  }

  return PreservedAnalyses::none();
}
