cmake_minimum_required(VERSION 3.4)
project(Obfuscator)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/env.cmake)
    message(STATUS "Use LLVM_HOME from env.cmake")
    include(${CMAKE_CURRENT_SOURCE_DIR}/env.cmake)
endif ()

# we need LLVM_HOME in order not automatically set LLVM_DIR
if (NOT DEFINED ENV{LLVM_HOME})
    message(FATAL_ERROR "$LLVM_HOME is not defined")
else ()
    set(ENV{LLVM_DIR} $ENV{LLVM_HOME}/lib64/cmake/llvm)
endif ()

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
if (${LLVM_VERSION_MAJOR} VERSION_GREATER_EQUAL 10)
    set(CMAKE_CXX_STANDARD 14)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
include_directories($ENV{LLVM_HOME}/include/c++v1)
set(CMAKE_SKIP_RPATH ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out")

include_directories(include)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions("-DDEBUG_MODE")
endif ()


set(COMMIT_HASH "")
set(COMMIT_VERSION "")
set(COMMIT_COUNT "")
find_package(Git QUIET)
if (GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%H
            OUTPUT_VARIABLE COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --abbrev=7 --dirty --always --tags
            OUTPUT_VARIABLE COMMIT_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
            OUTPUT_VARIABLE COMMIT_COUNT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif ()

message(STATUS "COMMIT_HASH ${COMMIT_HASH}")
message(STATUS "COMMIT_VERSION ${COMMIT_VERSION}")
message(STATUS "COMMIT_COUNT ${COMMIT_COUNT}")

configure_file(
        include/Version.h.in
        ${CMAKE_BINARY_DIR}/generated/Version.h
        @ONLY
)

include_directories(${CMAKE_BINARY_DIR}/generated)

add_library(Obfuscator MODULE # 生成动态库文件
        src/Plugin.cpp
        src/ObfuscationOptions.cpp

        src/utils/CryptoUtils.cpp
        src/utils/Utils.cpp

        src/core/HelloWorld.cpp
        )

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