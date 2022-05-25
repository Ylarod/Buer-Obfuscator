#ifndef OBFUSCATION_STRING_ENCRYPTION_H
#define OBFUSCATION_STRING_ENCRYPTION_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Obfuscation/CryptoUtils.h"
#include "llvm/Transforms/Obfuscation/IPObfuscationContext.h"
#include "llvm/Transforms/Obfuscation/ObfuscationOptions.h"

namespace llvm {

struct StringEncryption : public PassInfoMixin<StringEncryption> {
  bool flag;

  struct CSPEntry {
    CSPEntry()
        : ID(0), Offset(0), DecGV(nullptr), DecStatus(nullptr),
          DecFunc(nullptr) {}
    unsigned ID;
    unsigned Offset;
    GlobalVariable *DecGV;
    GlobalVariable *DecStatus; // is decrypted or not
    std::vector<uint8_t> Data;
    std::vector<uint8_t> EncKey;
    Function *DecFunc;
  };

  struct CSUser {
    CSUser(GlobalVariable *User, GlobalVariable *NewGV)
        : GV(User), DecGV(NewGV), DecStatus(nullptr), InitFunc(nullptr) {}
    GlobalVariable *GV;
    GlobalVariable *DecGV;
    GlobalVariable *DecStatus; // is decrypted or not
    Function *InitFunc; // InitFunc will use decryted string to initialize DecGV
  };

  IPObfuscationContext *IPO;
  ObfuscationOptions *Options;
  std::vector<CSPEntry *> ConstantStringPool;
  std::map<GlobalVariable *, CSPEntry *> CSPEntryMap;
  std::map<GlobalVariable *, CSUser *> CSUserMap;
  GlobalVariable *EncryptedStringTable;
  std::set<GlobalVariable *> MaybeDeadGlobalVars;

  StringEncryption(bool flag, IPObfuscationContext *IPO, ObfuscationOptions *Options) {
    this->flag = flag;
    this->IPO = IPO;
    this->Options = Options;
  }

  bool doFinalization(Module &) {
    for (CSPEntry *Entry : ConstantStringPool) {
      delete (Entry);
    }
    for (auto &I : CSUserMap) {
      CSUser *User = I.second;
      delete (User);
    }
    ConstantStringPool.clear();
    CSPEntryMap.clear();
    CSUserMap.clear();
    MaybeDeadGlobalVars.clear();
    return false;
  }

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &);

  void collectConstantStringUser(GlobalVariable *CString,
                                 std::set<GlobalVariable *> &Users);
  bool isValidToEncrypt(GlobalVariable *GV);
  bool isCString(const ConstantDataSequential *CDS);
  bool isObjCSelectorPtr(const GlobalVariable *GV);
  bool isCFConstantStringTag(const GlobalVariable *GV);
  bool processConstantStringUse(Function *F);
  void deleteUnusedGlobalVariable();
  Function *buildDecryptFunction(Module *M, const CSPEntry *Entry);
  Function *buildInitFunction(Module *M, const CSUser *User);
  void getRandomBytes(std::vector<uint8_t> &Bytes, uint32_t MinSize,
                      uint32_t MaxSize);
  void lowerGlobalConstant(Constant *CV, IRBuilder<> &IRB, Value *Ptr);
  void lowerGlobalConstantStruct(ConstantStruct *CS, IRBuilder<> &IRB,
                                 Value *Ptr);
  void lowerGlobalConstantArray(ConstantArray *CA, IRBuilder<> &IRB,
                                Value *Ptr);
};

}

#endif
