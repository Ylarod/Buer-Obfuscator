//
// Created by Ylarod on 2022/5/24.
//

#include "llvm/Transforms/Obfuscation/ObfuscatePlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Obfuscation/HelloWorld.h"

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

llvm::PassPluginLibraryInfo getLLVMObfuscationPluginInfo(){
  return {LLVM_PLUGIN_API_VERSION, "LLVMObfuscation", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(obfuscatePluginCallback);
          }};
}

namespace {
/* New PM Registration */
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  errs() << "llvmGetPassPluginInfo\n";
  return getLLVMObfuscationPluginInfo();
}

} // namespace