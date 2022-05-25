//
// Created by Ylarod on 2022/5/24.
//

#include "llvm/Transforms/Obfuscation/ObfuscatePlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Obfuscation/HelloWorld.h"
#include "llvm/Transforms/Obfuscation/ObfuscationPassManager.h"

using namespace llvm;

cl::opt<bool> RunHelloWorld("hello", cl::init(true),
                            cl::desc("Enable the HelloWorld pass"));

void obfuscatePluginCallback(llvm::ModulePassManager &PM,
                             llvm::OptimizationLevel Level) {
  FunctionPassManager FPM;
  FPM.addPass(HelloWorld(RunHelloWorld));
  PM.addPass(HelloWorld(RunHelloWorld));
  PM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
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