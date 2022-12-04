#include "utils/Utils.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <fmt/core.h>

namespace llvm {

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

    StringRef getEnvVar(const StringRef& key) {
        char const *value = getenv(key.data());
        return value == nullptr ? StringRef() : StringRef(value);
    }

    void fixStack(Function &F) {
        std::vector<PHINode *> origPHI;
        std::vector<Instruction *> origReg;
        BasicBlock &entryBB = F.getEntryBlock();
        for (BasicBlock &BB: F) {
            for (Instruction &I: BB) {
                if (auto *PN = dyn_cast<PHINode>(&I)) {
                    origPHI.push_back(PN);
                } else if (!(isa<AllocaInst>(&I) && I.getParent() == &entryBB) &&
                           I.isUsedOutsideOfBlock(&BB)) {
                    origReg.push_back(&I);
                }
            }
        }
        for (PHINode *PN: origPHI) {
            DemotePHIToStack(PN, entryBB.getTerminator());
        }
        for (Instruction *I: origReg) {
            DemoteRegToStack(*I, entryBB.getTerminator());
        }
    }

    std::string readAnnotate(GlobalObject *go) {
        std::string annotation;
        // Get annotation variable
        GlobalVariable *glob = go->getParent()->getGlobalVariable("llvm.global.annotations");
        if (glob == nullptr) {
            return "";
        }
        // Get the array
        auto *ca = dyn_cast<ConstantArray>(glob->getInitializer());
        if (ca == nullptr) {
            return "";
        }

        for (unsigned i = 0; i < ca->getNumOperands(); ++i) {
            // Get the struct
            auto *structAn = dyn_cast<ConstantStruct>(ca->getOperand(i));
            if (structAn == nullptr) {
                continue;
            }

            auto *expr = dyn_cast<ConstantExpr>(structAn->getOperand(0));
            if (expr == nullptr) {
                continue;
            }

            // If it's a bitcast we can check if the annotation is concerning
            // the current function
            if (expr->getOpcode() != Instruction::BitCast || expr->getOperand(0) != go) {
                continue;
            }

            auto *note = cast<ConstantExpr>(structAn->getOperand(1));
            // If it's a GetElementPtr, that means we found the variable
            // containing the annotations
            if (note->getOpcode() != Instruction::GetElementPtr) {
                continue;
            }

            auto *annotateStr = dyn_cast<GlobalVariable>(note->getOperand(0));
            if (annotateStr == nullptr) {
                continue;
            }

            auto *data = dyn_cast<ConstantDataSequential>(annotateStr->getInitializer());
            if (data == nullptr) {
                continue;
            }

            if (data->isString()) {
                annotation += data->getAsString().lower() + " ";
            }
        }
        return annotation;
    }

    bool toObfuscate(int flag, GlobalObject *go, const std::string& attribute) {
        const std::string& attr = attribute;
        std::string attrNo = "no-" + attr;

        // Check if declaration
        if (go->isDeclaration()) {
            return false;
        }

        // Check external linkage
        if (go->hasAvailableExternallyLinkage() != 0) {
            return false;
        }

        if (flag == 0){
            return false; // 强制关闭
        }

        std::string annotate = readAnnotate(go);
        if (annotate.find(attrNo) != std::string::npos) {
            return false;
        }
        if (annotate.find(attr) != std::string::npos) {
            return true;
        }

        if (flag == 1) {
            return false; // 白名单模式默认不开
        }else if(flag == 2){
            return true; // 黑名单模式默认开启
        }

        errs() << "\033[1;31m" << go->getName() << ": flag配置错误\n" << "\033[0m";
        abort();
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

    void printInst(Instruction *ins) {
        errs() << *ins << "\n";
    }


    void printBB(BasicBlock *BB) {
        errs() << BB->getName() << "\n";
        for (auto &i: *BB) {
            printInst(&i);
        }
        errs() << "\n";
    }

    void printFunction(Function *f) {
        errs() << "Function Name:" << f->getName() << "\n";
        for (auto &i: *f) {
            printBB(&i);
        }
    }

    void printModule(Module *m) {
        errs() << "Module Name:" << m->getName() << "\n";
        for (auto &i: *m) {
            printFunction(&i);
        }
    }

}

