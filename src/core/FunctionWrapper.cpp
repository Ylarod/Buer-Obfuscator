//
// Created by Ylarod on 2022/12/4.
//

#include "core/FunctionWrapper.h"
#include "utils/CryptoUtils.h"
#include "utils/Utils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include <fmt/color.h>
#include <fmt/core.h>
#include <llvm/IR/IRBuilder.h>
#include <vector>
#include <random>

using namespace llvm;
using std::vector;

PreservedAnalyses FunctionWrapper::run(Module &M, ModuleAnalysisManager &) const {
    PassFunctionWrapper &config = Options->FunctionWrapper;
    if (!config.enable) {
        return PreservedAnalyses::all();
    }
    vector<CallBase *> CallBases;
    for (auto &F: M) {
        if (!toObfuscate(config.enable, &F, "fw")) {
            IF_VERBOSE2 {
                outs() << fmt::format(fmt::fg(fmt::color::red),
                                      "FunctionWrapper: Ignore {}\n", F.getName().str());
            }
            continue;
        }
        for (auto &BB: F) {
            for (auto &I: BB) {
                if (isa<CallInst>(I) || isa<InvokeInst>(I)) {
                    if (config.prob == 100 || crypto->get_range(100) < (unsigned int) config.prob) {
                        auto &CB = cast<CallInst>(I);
                        CallBases.push_back(&CB);
                    }
                }
            }
        }
        IF_VERBOSE {
            outs() << fmt::format(fmt::fg(fmt::color::sky_blue),
                                  "FunctionWrapper: {}\n", F.getName().str());
        }
    }

    for (auto &CB: CallBases) {
        for (int i = 0; i < config.times; i++) {
            CB = HandleCallBase(CB);
        }
    }

    return PreservedAnalyses::all();
}

CallBase *FunctionWrapper::HandleCallBase(CallBase *CB) {
    if (CB == nullptr) {
        return nullptr;
    }
    if (CB->isIndirectCall()) {
        return nullptr;
    }
    Function *calledFunction = CB->getCalledFunction();
    if (calledFunction == nullptr) {
        return nullptr;
    }
    StringRef calledFunctionName = calledFunction->getName();
    if (calledFunctionName.startswith("clang.")) {
        return nullptr;
    }

    vector<Type *> types;
    for (unsigned i = 0; i < CB->arg_size(); i++) {
        types.push_back(CB->getArgOperand(i)->getType());
    }

    FunctionType *fTy = FunctionType::get(CB->getType(), ArrayRef<Type *>(types), false);

    std::string funcName;
    if (!calledFunctionName.startswith("Wra_")) {
        funcName = "Wra_" + calledFunctionName.str();
    } else {
        funcName = calledFunctionName.str();
    }
    Function *func = Function::Create(fTy,
                                      GlobalValue::LinkageTypes::InternalLinkage,
                                      Twine(funcName),
                                      CB->getParent()->getModule());
    appendToCompilerUsed(*func->getParent(), {func});

    func->copyAttributesFrom(cast<Function>(calledFunction));
    func->removeFnAttr(Attribute::AttrKind::AlwaysInline);
    func->addFnAttr(Attribute::AttrKind::NoInline);
    func->addFnAttr(Attribute::AttrKind::OptimizeNone);

    func->setDSOLocal(true);
    BasicBlock *BB = BasicBlock::Create(func->getContext(), "MainBB", func);
    IRBuilder<> builder(BB);

    if (fTy->getReturnType()->isVoidTy()) {
        builder.CreateRetVoid();
    } else {
        vector<Value *> params;
        for (auto& arg : func->args()) {
            params.push_back(&arg);
        }
        Constant *callee = ConstantExpr::getBitCast(calledFunction, calledFunction->getType());
        Value *ret = builder.CreateCall(
                cast<FunctionType>(callee->getType()->getPointerElementType()),
                callee,
                ArrayRef<Value *>(params));
        builder.CreateRet(ret);
    }
    CB->setCalledFunction(func);
    CB->mutateFunctionType(fTy);
    return CB;
}
