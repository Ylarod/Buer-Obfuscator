set(OBFUSCATOR_SOURCES
        Plugin.cpp
        ObfuscationOptions.cpp

        utils/CryptoUtils.cpp
        utils/Utils.cpp

        core/HelloWorld.cpp
        core/FuncNameObf.cpp
        core/GVNameObf.cpp
        core/FunctionWrapper.cpp
        )

if (OBFUSCATOR_IN_TREE_BUILDING)
#    set(LLVM_OBFUSCATOR_LINK_INTO_TOOLS ON)
    install(TARGETS fmt EXPORT LLVMExports)
    add_llvm_pass_plugin(Obfuscator
            ${OBFUSCATOR_SOURCES}
            DEPENDS
            intrinsics_gen
            BUILDTREE_ONLY
            )
else()
    add_library(Obfuscator MODULE # 生成动态库文件
            ${OBFUSCATOR_SOURCES})
    target_compile_features(Obfuscator PRIVATE
            cxx_range_for cxx_auto_type
            )
    set_target_properties(Obfuscator PROPERTIES
            COMPILE_FLAGS "-fno-rtti"
            )
endif ()

target_link_libraries(Obfuscator
        PRIVATE
        fmt)

if (APPLE)
    set_target_properties(Obfuscator PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif (APPLE)