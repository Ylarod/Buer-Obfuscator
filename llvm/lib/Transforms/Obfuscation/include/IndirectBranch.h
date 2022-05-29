#ifndef OBFUSCATION_INDIRECTBR_H
#define OBFUSCATION_INDIRECTBR_H

#include "CryptoUtils.h"
#include "IPObfuscationContext.h"
#include "ObfuscationOptions.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

// Namespace
namespace llvm {

struct IndirectBranch : public PassInfoMixin<IndirectBranch> {
  bool enable;

  IPObfuscationContext *IPO;
  ObfuscationOptions *Options;
  std::map<BasicBlock *, unsigned> BBNumbering;
  std::vector<BasicBlock *> BBTargets; // all conditional branch targets

  IndirectBranch(bool enable, IPObfuscationContext *IPO,
                 ObfuscationOptions *Options)
      : enable(enable), IPO(IPO), Options(Options) {}

  void NumberBasicBlock(Function &F);

  GlobalVariable *getIndirectTargets(Function &F, ConstantInt *EncKey);

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif
