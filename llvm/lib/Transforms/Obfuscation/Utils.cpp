#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

namespace llvm{

// Shamefully borrowed from ../Scalar/RegToMem.cpp :(
bool valueEscapes(Instruction *Inst) {
  BasicBlock *BB = Inst->getParent();
  for (Value::use_iterator UI = Inst->use_begin(), E = Inst->use_end(); UI != E;
       ++UI) {
    auto *I = cast<Instruction>(*UI);
    if (I->getParent() != BB || isa<PHINode>(I)) {
      return true;
    }
  }
  return false;
}

void fixStack(Function &F) {
  std::vector<PHINode *> origPHI;
  std::vector<Instruction *> origReg;
  BasicBlock &entryBB = F.getEntryBlock();
  do{
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (auto *PN = dyn_cast<PHINode>(&I)) {
          origPHI.push_back(PN);
        } else if (!(isa<AllocaInst>(&I) && I.getParent() == &entryBB) &&
                   I.isUsedOutsideOfBlock(&BB)) {
          origReg.push_back(&I);
        }
      }
    }
    for (PHINode *PN : origPHI) {
      DemotePHIToStack(PN, entryBB.getTerminator());
    }
    for (Instruction *I : origReg) {
      DemoteRegToStack(*I, entryBB.getTerminator());
    }
  }while(!origPHI.empty() || !origReg.empty());
}

std::string readAnnotate(Function *f) {
  std::string annotation;
  // Get annotation variable
  GlobalVariable *glob =
      f->getParent()->getGlobalVariable("llvm.global.annotations");
  if (glob != nullptr) {
    // Get the array
    if (auto *ca = dyn_cast<ConstantArray>(glob->getInitializer())) {
      for (unsigned i = 0; i < ca->getNumOperands(); ++i) {
        // Get the struct
        if (auto *structAn = dyn_cast<ConstantStruct>(ca->getOperand(i))) {
          if (auto *expr = dyn_cast<ConstantExpr>(structAn->getOperand(0))) {
            // If it's a bitcast we can check if the annotation is concerning
            // the current function
            if (expr->getOpcode() == Instruction::BitCast &&
                expr->getOperand(0) == f) {
              auto *note = cast<ConstantExpr>(structAn->getOperand(1));
              // If it's a GetElementPtr, that means we found the variable
              // containing the annotations
              if (note->getOpcode() == Instruction::GetElementPtr) {
                if (auto *annotateStr =
                        dyn_cast<GlobalVariable>(note->getOperand(0))) {
                  if (auto *data = dyn_cast<ConstantDataSequential>(
                          annotateStr->getInitializer())) {
                    if (data->isString()) {
                      annotation += data->getAsString().lower() + " ";
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return annotation;
}

bool toObfuscate(bool flag, Function *f, std::string attribute) {
  std::string attr = attribute;
  std::string attrNo = "no" + attr;

  // Check if declaration
  if (f->isDeclaration()) {
    return false;
  }

  // Check external linkage
  if (f->hasAvailableExternallyLinkage() != 0) {
    return false;
  }

  // We have to check the nofla flag first
  // Because .find("fla") is true for a string like "fla" or
  // "nofla"
  if (readAnnotate(f).find(attrNo) != std::string::npos) {
    return false;
  }

  // If fla annotations
  if (readAnnotate(f).find(attr) != std::string::npos) {
    return true;
  }

  // If fla flag is set
  if (flag == true) {
    /* Check if the number of applications is correct
    if (!((Percentage > 0) && (Percentage <= 100))) {
      LLVMContext &ctx = llvm::getGlobalContext();
      ctx.emitError(Twine("Flattening application function\
              percentage -perFLA=x must be 0 < x <= 100"));
    }
    // Check name
    else if (func.size() != 0 && func.find(f->getName()) != std::string::npos) {
      return true;
    }

    if ((((int)llvm::cryptoutils->get_range(100))) < Percentage) {
      return true;
    }
    */
    return true;
  }

  return false;
}

void LowerConstantExpr(Function &F) {
  SmallPtrSet<Instruction *, 8> WorkList;

  for (inst_iterator It = inst_begin(F), E = inst_end(F); It != E; ++It) {
    Instruction *I = &*It;

    if (isa<LandingPadInst>(I))
      continue;
    if (auto *II = dyn_cast<IntrinsicInst>(I)) {
      if (II->getIntrinsicID() == Intrinsic::eh_typeid_for) {
        continue;
      }
    }

    for (unsigned int i = 0; i < I->getNumOperands(); ++i) {
      if (isa<ConstantExpr>(I->getOperand(i)))
        WorkList.insert(I);
    }
  }

  while (!WorkList.empty()) {
    auto It = WorkList.begin();
    Instruction *I = *It;
    WorkList.erase(*It);

    if (auto *PHI = dyn_cast<PHINode>(I)) {
      for (unsigned int i = 0; i < PHI->getNumIncomingValues(); ++i) {
        Instruction *TI = PHI->getIncomingBlock(i)->getTerminator();
        if (auto *CE = dyn_cast<ConstantExpr>(PHI->getIncomingValue(i))) {
          Instruction *NewInst = CE->getAsInstruction();
          NewInst->insertBefore(TI);
          PHI->setIncomingValue(i, NewInst);
          WorkList.insert(NewInst);
        }
      }
    } else {
      for (unsigned int i = 0; i < I->getNumOperands(); ++i) {
        if (auto *CE = dyn_cast<ConstantExpr>(I->getOperand(i))) {
          Instruction *NewInst = CE->getAsInstruction();
          NewInst->insertBefore(I);
          I->replaceUsesOfWith(CE, NewInst);
          WorkList.insert(NewInst);
        }
      }
    }
  }
}

}

