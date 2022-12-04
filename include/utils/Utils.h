#ifndef OBFUSCATOR_UTILS_H
#define OBFUSCATOR_UTILS_H

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Transforms/Utils/Local.h> // For DemoteRegToStack and DemotePHIToStack

namespace llvm {
    bool valueEscapes(Instruction *Inst);

    StringRef getEnvVar(const StringRef& key);

    void fixStack(Function &F);

    std::string readAnnotate(Function *f);

    bool toObfuscate(int flag, Function *f, std::string attribute);

    void LowerConstantExpr(Function &F);

    void printInst(Instruction *ins);

    void printBB(BasicBlock *BB);

    void printFunction(Function *f);

    void printModule(Module *m);
}

#endif
