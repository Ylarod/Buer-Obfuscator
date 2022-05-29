#ifndef OBFUSCATION_INDIRECT_GLOBAL_VARIABLE_H
#define OBFUSCATION_INDIRECT_GLOBAL_VARIABLE_H

#include "CryptoUtils.h"
#include "IPObfuscationContext.h"
#include "ObfuscationOptions.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

// Namespace
namespace llvm {

struct IndirectGlobalVariable : public PassInfoMixin<IndirectGlobalVariable> {
  bool enable;

  IPObfuscationContext *IPO;
  ObfuscationOptions *Options;
  std::map<GlobalVariable *, unsigned> GVNumbering;
  std::vector<GlobalVariable *> GlobalVariables;

  IndirectGlobalVariable(bool enable, IPObfuscationContext *IPO,
                         ObfuscationOptions *Options)
      : enable(enable), IPO(IPO), Options(Options) {}

  void NumberGlobalVariable(Function &F);

  GlobalVariable *getIndirectGlobalVariables(Function &F, ConstantInt *EncKey);

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif
