#ifndef OBFUSCATION_INDIRECT_GLOBAL_VARIABLE_H
#define OBFUSCATION_INDIRECT_GLOBAL_VARIABLE_H

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Obfuscation/CryptoUtils.h"
#include "llvm/Transforms/Obfuscation/IPObfuscationContext.h"
#include "llvm/Transforms/Obfuscation/ObfuscationOptions.h"

// Namespace
namespace llvm {

struct IndirectGlobalVariable : public PassInfoMixin<IndirectGlobalVariable> {
  bool enable;

  IPObfuscationContext *IPO;
  ObfuscationOptions *Options;
  std::map<GlobalVariable *, unsigned> GVNumbering;
  std::vector<GlobalVariable *> GlobalVariables;

  IndirectGlobalVariable(bool enable, IPObfuscationContext *IPO, ObfuscationOptions *Options){
    this->enable = enable;
    this->IPO = IPO;
    this->Options = Options;
  }

  void NumberGlobalVariable(Function &F);

  GlobalVariable *getIndirectGlobalVariables(Function &F, ConstantInt *EncKey);

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

};

}

#endif
