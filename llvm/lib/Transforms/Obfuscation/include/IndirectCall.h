#ifndef OBFUSCATION_INDIRECT_CALL_H
#define OBFUSCATION_INDIRECT_CALL_H

#include "CryptoUtils.h"
#include "IPObfuscationContext.h"
#include "ObfuscationOptions.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

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
