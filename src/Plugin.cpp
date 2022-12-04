//
// Created by Ylarod on 2022/5/24.
//

#include "Plugin.h"
#include "ObfuscationOptions.h"
#include "core/HelloWorld.h"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <utils/Utils.h>
#include "Version.h"

using namespace llvm;

cl::opt<std::string> ConfigFile("obf-cfg", cl::desc("Obfuscator configuration file"), cl::Optional);

static ObfuscationOptions *getOptions() {
    // 优先级： 命令行配置 > 命令行配置文件 > 环境变量配置文件 > home目录配置文件 > 默认配置
    ObfuscationOptions *Options;
    if (sys::fs::exists(ConfigFile.getValue())) {
        Options = new ObfuscationOptions(ConfigFile.getValue());
    } else {
        StringRef envConfig = getEnvVar("OBF_CONFIG_FILE");
        if (!envConfig.empty()){
            Options = new ObfuscationOptions(envConfig);
        }else{
            SmallString<128> ConfigurePath;
            if (sys::path::home_directory(ConfigurePath)) {
                sys::path::append(ConfigurePath, ".buer_obfuscator");
                Options = new ObfuscationOptions(ConfigurePath);
            } else {
                Options = new ObfuscationOptions();
            }
        }
    }
    return Options;
}


void obfuscatePluginCallback(llvm::ModulePassManager &PM,
                             llvm::OptimizationLevel Level) {
    ObfuscationOptions *Options = getOptions();
    Options->dump();
    FunctionPassManager FPM;
    FPM.addPass(HelloWorld(Options->HelloWorld.enable));
    PM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
    PM.addPass(HelloWorld(Options->HelloWorld.enable));
}

/* New PM Registration for static plugin */
llvm::PassPluginLibraryInfo getLLVMObfuscationPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "Buer", obf_version_name,
            [](PassBuilder &PB) {
                dbgs() << "\033[1;35m" << "Buer Obfuscator v" << obf_version_name << "\n" << "\033[0m";
                PB.registerPipelineStartEPCallback(obfuscatePluginCallback);
            }};
}


/* New PM Registration for dynamic plugin */

#ifndef LLVM_LLVMOBFUSCATION_LINK_INTO_TOOLS
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getLLVMObfuscationPluginInfo();
}
#endif