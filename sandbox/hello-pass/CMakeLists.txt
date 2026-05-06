cmake_minimum_required(VERSION 3.16)
project(HelloWorldPass)

# ====== 指向你的 LLVM 15 构建目录 ======
set(LLVM_DIR "/root/graduation/build-llvm15/lib/cmake/llvm" CACHE PATH "" FORCE)

find_package(LLVM 15 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# LLVM 通常不开启 RTTI，需要保持一致
if(NOT LLVM_ENABLE_RTTI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

add_library(HelloWorldPass MODULE HelloWorldPass.cpp)

set_target_properties(HelloWorldPass PROPERTIES
    PREFIX ""
    SUFFIX ".so"
)
