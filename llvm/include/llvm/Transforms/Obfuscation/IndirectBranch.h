#ifndef OBFUSCATION_INDIRECTBR_H
#define OBFUSCATION_INDIRECTBR_H

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Obfuscation/CryptoUtils.h"
#include "llvm/Transforms/Obfuscation/IPObfuscationContext.h"
#include "llvm/Transforms/Obfuscation/ObfuscationOptions.h"

// Namespace
namespace llvm {

struct IndirectBranch : public PassInfoMixin<IndirectBranch> {
  bool enable;

  IPObfuscationContext *IPO;
  ObfuscationOptions *Options;
  std::map<BasicBlock *, unsigned> BBNumbering;
  std::vector<BasicBlock *> BBTargets;        //all conditional branch targets

  IndirectBranch(bool flag, IPObfuscationContext *IPO, ObfuscationOptions *Options) {
    this->enable = flag;
    this->IPO = IPO;
    this->Options = Options;
  }

  void NumberBasicBlock(Function &F);

  GlobalVariable *getIndirectTargets(Function &F, ConstantInt *EncKey);

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

};

}

#endif
