cmake_minimum_required(VERSION 3.16.3)
project(hex-processor)
include(GNUInstallDirs)

# Boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.68.0 REQUIRED COMPONENTS
             filesystem
             unit_test_framework)

# Python
find_package(Python3 REQUIRED COMPONENTS
             Interpreter)

# Use local find scripts
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

option(USE_VERILATOR "Use Verilator for simulation" ON)
option(USE_YOSYS     "Use Yosys for synthesis" ON)
option(BUILD_DOCS    "Create and install HTML documentation" OFF)

# Verilator
if (USE_VERILATOR)
  find_package(verilator HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT})
  if (NOT verilator_FOUND)
    message(FATAL_ERROR "Verilator was not found.")
  endif()
  message(STATUS "Found Verilator binary = ${VERILATOR_BIN}")
endif()

# Yosys
if (USE_YOSYS)
  find_program(YOSYS_BIN yosys HINTS $ENV{YOSYS_ROOT} ${YOSYS_ROOT})
  if (NOT YOSYS_BIN)
    message(FATAL_ERROR "Yosys was not found.")
  endif()
  message(STATUS "Found Yosys binary = ${YOSYS_BIN}")
endif()

# Compiler config
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -pedantic)

include_directories(${Boost_INCLUDE_DIRS})

# Simulator
add_executable(hexsim hex.cpp hexsim.cpp)
target_link_libraries(hexsim ${Boost_LIBRARIES})

# Assembler
add_executable(hexasm hex.cpp hexasm.cpp)
target_link_libraries(hexasm ${Boost_LIBRARIES})

# Compiler
add_executable(xcmp hex.cpp xcmp.cpp)
target_link_libraries(xcmp ${Boost_LIBRARIES})

# Compile and run
add_executable(xrun hex.cpp xrun.cpp)
target_link_libraries(xrun ${Boost_LIBRARIES})

install(TARGETS hexasm xcmp xrun hexsim
        DESTINATION ${CMAKE_INSTALL_BINDIR})

# Verilator
if (USE_VERILATOR)
  add_executable(hextb hex.cpp hextb.cpp)

  verilate(hextb TRACE
           VERILATOR_ARGS --top-module hex
           SOURCES verilog/hex_pkg.sv
                   verilog/hex.sv
                   verilog/processor.sv
                   verilog/memory.sv)

  install(TARGETS hextb
          DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

enable_testing()
add_subdirectory(synth)
add_subdirectory(tests)

if (BUILD_DOCS)
  add_subdirectory(docs)
endif()
