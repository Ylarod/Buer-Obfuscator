add_llvm_library(LLVMObfuscation
        ObfuscatePlugin.cpp
        HelloWorld.cpp

        DEPENDS
        intrinsics_gen

        LINK_COMPONENTS
        Core
        Support
        Analysis
        TransformUtils
        )