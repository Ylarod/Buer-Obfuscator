# Buer-Obfuscator

A WIP Obfuscator based on llvm14.

> The name, **Buer**, comes from a character of Genshin Impact, also known as **Nahida**

## Notes

Develop based on ndk-r25b, source can be found in [ndk-r25b](https://github.com/Ylarod/Buer-Obfuscator/tree/ndk-r25b) (Without commit history)

## Features

1. Function Name Obfuscate
2. Global Variable Name Obfuscate

## Usage

### Building

1. Set LLVM_HOME environment variable or add following code to env.cmake

    ```cmake
    set(ENV{LLVM_HOME} /path/to/llvm)
    ```

2. Build it.

## Development Document

**Mostly Chinese only**

1. [docs](https://github.com/Ylarod/Buer-Obfuscator/tree/main/docs)
2. [Ylarod's Blog](https://xtuly.cn/tag/LLVM)