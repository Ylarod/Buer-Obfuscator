//
// Created by Ylarod on 2022/5/24.
//

#ifndef LLVM_OBFUSCATEPLUGIN_H
#define LLVM_OBFUSCATEPLUGIN_H

#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"

extern void obfuscatePluginCallback(llvm::ModulePassManager &PM, llvm::OptimizationLevel Level);

#endif // LLVM_OBFUSCATEPLUGIN_H
