cmake_minimum_required(VERSION 3.16)
project(ControlFlowObfPass)


set(LLVM_DIR "/home/hang/graduationDesign/toolChain/build-llvm15/lib/cmake/llvm" CACHE PATH "" FORCE)
find_package(LLVM 15 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")


include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})


if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()


add_library(ControlFlowObfPass MODULE src/ControlFlowObf.cpp)
set_target_properties(ControlFlowObfPass PROPERTIES
    PREFIX "" SUFFIX ".so"
    LINK_FLAGS "-Wl,-undefined,dynamic_lookup")
