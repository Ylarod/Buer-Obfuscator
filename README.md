# Obfuscator

A WIP Obfuscator based on android clang14, ndk 24.0.8215888

The project name has not been decided yet, the repo name may be changed in the future.

## Progress

- [x] 1. port goron


## Environment
This project was developed and tested on the following environment:
- macOS Monterey 12.4 aarch64
- Apple clang version 13.1.6 (clang-1316.0.21.2.5)
- CMake 3.23.1
- Ninja 1.10.2




## Usage

### Building on Linux/Windows
The following commands work on both Linux and Windows:

```shell
cd build
cmake -G "Ninja" -DLLVM_ENABLE_PROJECTS="clang" \
    -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="X86" \
    -DBUILD_SHARED_LIBS=On ../llvm
ninja
```

### Building on MacOS

```shell
mkdir -p build
cd build
cmake -G "Ninja" -DLLVM_ENABLE_PROJECTS="clang" \
    -DCMAKE_BUILD_TYPE=Release \
    -DDEFAULT_SYSROOT=$(xcrun --show-sdk-path) \
    -DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX11.3.sdk \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    ../llvm
ninja
```
