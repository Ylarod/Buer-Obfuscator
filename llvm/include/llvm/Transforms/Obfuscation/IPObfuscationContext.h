#ifndef OBFUSCATION_IPOBFUSCATIONCONTEXT_H
#define OBFUSCATION_IPOBFUSCATIONCONTEXT_H

#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Obfuscation/CryptoUtils.h"
#include <set>

// Namespace
namespace llvm {
class ModulePass;
class FunctionPass;
class PassRegistry;

struct IPObfuscationContext : public PassInfoMixin<IPObfuscationContext> {
  bool enable;

  /* Inter-procedural obfuscation secret info of a function */
  struct IPOInfo {
    IPOInfo(AllocaInst *CallerAI, AllocaInst *CalleeAI, LoadInst *LI, ConstantInt *Value)
        : CallerSlot(CallerAI), CalleeSlot(CalleeAI), SecretLI(LI), SecretCI(Value) {}
    // Stack slot use to store caller's secret token
    AllocaInst *CallerSlot;
    // Stack slot use to store callee's secret argument
    AllocaInst *CalleeSlot;
    // Load caller secret from caller's slot or the secret argument passed by caller
    LoadInst *SecretLI;
    // A random constant value
    ConstantInt *SecretCI;
  };

  CryptoUtils RandomEngine;
  std::set<Function *> LocalFunctions;
  SmallVector<IPOInfo *, 16> IPOInfoList;
  std::map<Function *, IPOInfo *> IPOInfoMap;
  std::vector<AllocaInst *> DeadSlots;

  IPObfuscationContext() : enable(false) {}
  explicit IPObfuscationContext(bool enable) : enable(enable) {}
  IPObfuscationContext(bool enable, const std::string& seed);

  void SurveyFunction(Function &F);
  Function *InsertSecretArgument(Function *F);
  void computeCallSiteSecretArgument(Function *F);
  IPOInfo* AllocaSecretSlot(Function &F);
  const IPOInfo *getIPOInfo(Function *F);

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &);
};

}

#endif
