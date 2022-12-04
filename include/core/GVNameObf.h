//
// Created by Ylarod on 2022/12/4.
//

#ifndef OBFUSCATOR_GVNAMEOBF_H
#define OBFUSCATOR_GVNAMEOBF_H

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include "ObfuscationOptions.h"

namespace llvm {

    class GVNameObf : public PassInfoMixin<GVNameObf> {
        ObfuscationOptions* Options;

    public:
        explicit GVNameObf(ObfuscationOptions* Options) : Options(Options) {}

        PreservedAnalyses run(Module &M, ModuleAnalysisManager &) const;
    };

} // namespace llvm

#endif //OBFUSCATOR_GVNAMEOBF_H
