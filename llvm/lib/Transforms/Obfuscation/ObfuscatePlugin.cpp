//
// Created by Ylarod on 2022/5/24.
//

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Path.h"
#include "llvm/Transforms/Obfuscation/ObfuscatePlugin.h"
#include "llvm/Transforms/Obfuscation/HelloWorld.h"
#include "llvm/Transforms/Obfuscation/IndirectBranch.h"
#include "llvm/Transforms/Obfuscation/IndirectCall.h"
#include "llvm/Transforms/Obfuscation/IndirectGlobalVariable.h"
#include "llvm/Transforms/Obfuscation/Flattening.h"
#include "llvm/Transforms/Obfuscation/StringEncryption.h"
#include "llvm/Transforms/Obfuscation/IPObfuscationContext.h"

using namespace llvm;

cl::opt<bool> RunHelloWorld("hello", cl::init(false),
                            cl::desc("Enable the HelloWorld pass"));

cl::opt<bool>
    EnableIndirectBr("irobf-indbr", cl::init(false), 
                     cl::desc("Enable IR Indirect Branch Obfuscation."));

cl::opt<bool>
    EnableIndirectCall("irobf-icall", cl::init(false), 
                       cl::desc("Enable IR Indirect Call Obfuscation."));

cl::opt<bool>
    EnableIndirectGV("irobf-indgv", cl::init(false), 
                     cl::desc("Enable IR Indirect Global Variable Obfuscation."));

cl::opt<bool>
    EnableIRFlattening("irobf-cff", cl::init(false), 
                       cl::desc("Enable IR Control Flow Flattening Obfuscation."));

cl::opt<bool>
    EnableIRStringEncryption("irobf-cse", cl::init(false), 
                             cl::desc("Enable IR Constant String Encryption."));

cl::opt<std::string>
    GoronConfigure("goron-cfg", cl::desc("Goron configuration file"), cl::Optional);

static ObfuscationOptions *getOptions() {
  ObfuscationOptions *Options = nullptr;
  if (sys::fs::exists(GoronConfigure.getValue())) {
    Options = new ObfuscationOptions(GoronConfigure.getValue());
  } else {
    SmallString<128> ConfigurePath;
    if (sys::path::home_directory(ConfigurePath)) {
      sys::path::append(ConfigurePath, "goron.yaml");
      Options = new ObfuscationOptions(ConfigurePath);
    } else {
      Options = new ObfuscationOptions();
    }
  }
  return Options;
}

struct ObfuscationPassManager : public ModulePassManager {
  ObfuscationPassManager(){
    bool enableIPO = EnableIRFlattening || EnableIndirectBr || EnableIndirectCall || EnableIndirectGV;
    addPass(IPObfuscationContext(enableIPO));
  }

  IPObfuscationContext* getIPObfuscationContext(){
    using IPObfuscationContextPassModel =
        detail::PassModel<Module, IPObfuscationContext, PreservedAnalyses, AnalysisManager<Module>>;
    auto *P = (IPObfuscationContextPassModel*)Passes[0].get();
    return &P->Pass;
  }
};

void obfuscatePluginCallback(llvm::ModulePassManager &PM,
                             llvm::OptimizationLevel Level) {

  ObfuscationPassManager OPM;
  IPObfuscationContext* pIPO = OPM.getIPObfuscationContext();
  ObfuscationOptions* Options = getOptions();

  OPM.addPass(StringEncryption(EnableIRStringEncryption || Options->EnableCSE, pIPO, Options));
  FunctionPassManager FPM;
  FPM.addPass(HelloWorld(RunHelloWorld));
  FPM.addPass(Flattening(EnableIRFlattening || Options->EnableCFF, pIPO, Options));
  FPM.addPass(IndirectBranch(EnableIndirectBr || Options->EnableIndirectBr, pIPO, Options));
  FPM.addPass(IndirectCall(EnableIndirectCall || Options->EnableIndirectCall, pIPO, Options));
  FPM.addPass(IndirectGlobalVariable(EnableIndirectGV || Options->EnableIndirectGV, pIPO, Options));
  OPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

  PM.addPass(HelloWorld(RunHelloWorld));
  PM.addPass(std::move(OPM));
}

/* New PM Registration for static plugin */
llvm::PassPluginLibraryInfo getLLVMObfuscationPluginInfo(){
  return {LLVM_PLUGIN_API_VERSION, "LLVMObfuscation", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(obfuscatePluginCallback);
          }};
}

#ifndef LLVM_LLVMOBFUSCATION_LINK_INTO_TOOLS
/* New PM Registration for dynamic plugin */
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  errs() << "llvmGetPassPluginInfo\n";
  return getLLVMObfuscationPluginInfo();
}
#endif