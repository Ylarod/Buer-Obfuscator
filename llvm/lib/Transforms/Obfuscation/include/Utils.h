#ifndef __UTILS_OBF__
#define __UTILS_OBF__

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/Local.h" // For DemoteRegToStack and DemotePHIToStack

namespace llvm{
bool valueEscapes(Instruction *Inst);
void fixStack(Function &F);
std::string readAnnotate(Function *f);
bool toObfuscate(bool flag, Function *f, std::string attribute);
void LowerConstantExpr(Function &F);
void printInst(Instruction* ins);
void printBB(BasicBlock* BB);
void printFunction(Function* f);
void printModule(Module* m);
}

#endif
