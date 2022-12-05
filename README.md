# Buer-Obfuscator

A WIP Obfuscator based on llvm14.

> The name, **Buer**, comes from a character of Genshin Impact, also known as **Nahida**

# Notes

Develop based on ndk-r25b, source can be found in [ndk-r25b](https://github.com/Ylarod/Buer-Obfuscator/tree/ndk-r25b) (Without commit history)

# Features

1. Function Name Obfuscate
2. Global Variable Name Obfuscate

# Usage

## Building

### Out-of-tree Building

1. Set LLVM_HOME environment variable or add following code to env.cmake

    ```cmake
    set(ENV{LLVM_HOME} /path/to/llvm)
    ```

2. Build it.

### In-tree Building

1. Patch **llvm-project/llvm/CMakeLists.txt**

   ```text
   Index: llvm/CMakeLists.txt
   IDEA additional info:
   Subsystem: com.intellij.openapi.diff.impl.patch.CharsetEP
   <+>UTF-8
   ===================================================================
   diff --git a/llvm/CMakeLists.txt b/llvm/CMakeLists.txt
   --- a/llvm/CMakeLists.txt	(revision 73fc653f7946653a0ed13958fd8fa5391024bd46)
   +++ b/llvm/CMakeLists.txt	(date 1670200126146)
   @@ -1094,6 +1094,8 @@
      add_subdirectory(examples)
    endif()
    
   +add_subdirectory(../../Obfuscator Obfuscator)
   +
    if( LLVM_INCLUDE_TESTS )
      if(EXISTS ${LLVM_MAIN_SRC_DIR}/projects/test-suite AND TARGET clang)
        include(LLVMExternalProjectUtils)
   
   ```
   
2. If you want to build static plugin, uncomment below in **src/CMakeLists.txt**

   ```cmake
   #    set(LLVM_OBFUSCATOR_LINK_INTO_TOOLS ON)
   ```
   
3. Build LLVM

   ```shell
   mkdir build && cd build
   cmake -G "Ninja" \
   -DLLVM_ENABLE_PROJECTS="clang" \
   -DCMAKE_BUILD_TYPE=Debug \
   -DDEFAULT_SYSROOT=$(xcrun --show-sdk-path) \
   -DCMAKE_OSX_SYSROOT=$(xcrun --show-sdk-path) \
   -DCMAKE_OSX_ARCHITECTURES="arm64" ..
   ninja
   ```

# Debug

1. Run `clang -v -fpass-plugin=libObfuscator.so -Xclang -load -Xclang libObfuscator.so test.cpp -o test`
2. Get real clang command, which starts with `"/path/to/clang" -cc1`

> https://stackoverflow.com/questions/25076625/lldb-not-stopping-on-my-breakpoint

# Development Document

**Mostly Chinese only**

1. [docs](https://github.com/Ylarod/Buer-Obfuscator/tree/main/docs)
2. [Ylarod's Blog](https://xtuly.cn/tag/LLVM)