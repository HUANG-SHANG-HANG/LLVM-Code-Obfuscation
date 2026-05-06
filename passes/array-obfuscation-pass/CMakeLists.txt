cmake_minimum_required(VERSION 3.16)
project(ArrayObfPass)


set(LLVM_DIR "/home/hang/graduationDesign/toolChain/build-llvm15/lib/cmake/llvm" CACHE PATH "" FORCE)
find_package(LLVM 15 REQUIRED CONFIG)


include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})


if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()


add_library(ArrayObfPass MODULE src/ArrayObf.cpp)
set_target_properties(ArrayObfPass PROPERTIES
    PREFIX "" SUFFIX ".so"
    LINK_FLAGS "-Wl,-undefined,dynamic_lookup")
