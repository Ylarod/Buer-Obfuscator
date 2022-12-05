//
// Created by Ylarod on 2022/12/4.
//

#ifndef OBFUSCATOR_FUNCTIONWRAPPER_H
#define OBFUSCATOR_FUNCTIONWRAPPER_H

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include "ObfuscationOptions.h"

namespace llvm {

    class FunctionWrapper : public PassInfoMixin<FunctionWrapper> {
        ObfuscationOptions* Options;

    public:
        explicit FunctionWrapper(ObfuscationOptions* Options) : Options(Options) {}

        PreservedAnalyses run(Module &M, ModuleAnalysisManager &) const;

        static CallBase* HandleCallBase(CallBase* CB);
    };

} // namespace llvm
#endif //OBFUSCATOR_FUNCTIONWRAPPER_H
