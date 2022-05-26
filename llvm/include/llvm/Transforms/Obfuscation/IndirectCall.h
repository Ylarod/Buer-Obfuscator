#ifndef OBFUSCATION_INDIRECT_CALL_H
#define OBFUSCATION_INDIRECT_CALL_H

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Obfuscation/CryptoUtils.h"
#include "llvm/Transforms/Obfuscation/IPObfuscationContext.h"
#include "llvm/Transforms/Obfuscation/ObfuscationOptions.h"

// Namespace
namespace llvm {
struct IndirectCall : public PassInfoMixin<IndirectCall> {
  bool enable;

  IPObfuscationContext *IPO;
  ObfuscationOptions *Options;
  std::map<Function *, unsigned> CalleeNumbering;
  std::vector<CallInst *> CallSites;
  std::vector<Function *> Callees;

  IndirectCall(bool enable, IPObfuscationContext *IPO,
               ObfuscationOptions *Options)
      : enable(enable), IPO(IPO), Options(Options) {}

  void NumberCallees(Function &F);

  GlobalVariable *getIndirectCallees(Function &F, ConstantInt *EncKey);

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};
} // namespace llvm

#endif
