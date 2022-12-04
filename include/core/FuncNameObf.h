//
// Created by Ylarod on 2022/12/4.
//

#ifndef OBFUSCATOR_FUNCNAMEOBF_H
#define OBFUSCATOR_FUNCNAMEOBF_H

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include "ObfuscationOptions.h"

namespace llvm {

    class FuncNameObf : public PassInfoMixin<FuncNameObf> {
        ObfuscationOptions* Options;

    public:
        explicit FuncNameObf(ObfuscationOptions* Options) : Options(Options) {}

        PreservedAnalyses run(Module &M, ModuleAnalysisManager &) const;
    };

} // namespace llvm

#endif //OBFUSCATOR_FUNCNAMEOBF_H
