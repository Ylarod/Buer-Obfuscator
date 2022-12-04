add_library(Obfuscator MODULE # 生成动态库文件
        Plugin.cpp
        ObfuscationOptions.cpp

        utils/CryptoUtils.cpp
        utils/Utils.cpp

        core/HelloWorld.cpp
        core/FuncNameObf.cpp
        core/GVNameObf.cpp)

target_link_libraries(Obfuscator
        fmt)

# Use C++11 to compile your pass (i.e., supply -std=c++11).
target_compile_features(Obfuscator PRIVATE
        cxx_range_for cxx_auto_type
        )

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(Obfuscator PROPERTIES
        COMPILE_FLAGS "-fno-rtti"
        )

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if (APPLE)
    set_target_properties(Obfuscator PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif (APPLE)