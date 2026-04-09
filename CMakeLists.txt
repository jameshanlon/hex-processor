cmake_minimum_required(VERSION 3.14.0)
project(hex-processor)
include(GNUInstallDirs)

# FetchContent for fmt
include(FetchContent)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 11.0.2)
FetchContent_MakeAvailable(fmt)

# FetchContent for Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0)
FetchContent_MakeAvailable(Catch2)

# Python
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Use local find scripts
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

option(USE_VERILATOR "Use Verilator for simulation" ON)
option(BUILD_DOCS "Create and install HTML documentation" OFF)

# Verilator
if(USE_VERILATOR)
  find_package(verilator HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT})
  if(NOT verilator_FOUND)
    message(FATAL_ERROR "Verilator was not found.")
  endif()
  message(STATUS "Found Verilator binary = ${VERILATOR_BIN}")
endif()

# Compiler config
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -pedantic)

# Simulator
add_executable(hexsim hex.cpp hexsim.cpp)
target_link_libraries(hexsim fmt::fmt)

# Assembler
add_executable(hexasm hex.cpp hexasm.cpp)
target_link_libraries(hexasm fmt::fmt)

# Compiler
add_executable(xcmp hex.cpp xcmp.cpp)
target_link_libraries(xcmp fmt::fmt)

# Compile and run
add_executable(xrun hex.cpp xrun.cpp)
target_link_libraries(xrun fmt::fmt)

install(TARGETS hexasm xcmp xrun hexsim DESTINATION ${CMAKE_INSTALL_BINDIR})

# Verilator
if(USE_VERILATOR)
  add_executable(hextb hex.cpp hextb.cpp)

  verilate(
    hextb
    TRACE
    VERILATOR_ARGS
    --top-module
    hex
    SOURCES
    verilog/hex_pkg.sv
    verilog/hex.sv
    verilog/processor.sv
    verilog/memory.sv)

  install(TARGETS hextb DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

enable_testing()
add_subdirectory(tests)

if(BUILD_DOCS)
  add_subdirectory(docs)
endif()
