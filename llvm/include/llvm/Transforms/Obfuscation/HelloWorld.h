//
// Created by Ylarod on 2022/5/24.
//

#ifndef LLVM_HELLOWORLD_H
#define LLVM_HELLOWORLD_H

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

namespace llvm {

class HelloWorld : public PassInfoMixin<HelloWorld> {
  bool enable;

public:
  explicit HelloWorld(bool enable = true) : enable(enable) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) const;

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) const;
};

} // namespace llvm

#endif // LLVM_HELLOWORLD_H
