set(LLVM_LLVMOBFUSCATION_LINK_INTO_TOOLS ON)

if (NOT WIN32)
    add_llvm_pass_plugin(LLVMObfuscation
            ObfuscatePlugin.cpp
            HelloWorld.cpp
            CryptoUtils.cpp
            Utils.cpp
            ObfuscationPassManager.cpp
            ObfuscationOptions.cpp
            IPObfuscationContext.cpp
            IndirectBranch.cpp
            IndirectCall.cpp
            IndirectGlobalVariable.cpp
            Flattening.cpp
            StringEncryption.cpp
            DEPENDS
            intrinsics_gen
            BUILDTREE_ONLY
            )
    install(TARGETS ${name} RUNTIME DESTINATION examples)
    set_target_properties(${name} PROPERTIES FOLDER "Examples")
endif()
