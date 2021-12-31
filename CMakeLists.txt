cmake_minimum_required(VERSION 3.21.0)
project(hex-processor)
include(GNUInstallDirs)

# Boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.68.0 REQUIRED COMPONENTS
             unit_test_framework)

# Python
find_package(Python3 REQUIRED COMPONENTS
             Interpreter)

# Verilator
find_package(verilator HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT})
if (NOT verilator_FOUND)
  message(FATAL_ERROR "Verilator was not found.")
endif()
message(STATUS "Found Verilator binary = ${VERILATOR_BIN}")

# Yosys
find_program(YOSYS_BIN yosys HINTS $ENV{YOSYS_ROOT} ${YOSYS_ROOT})
if (NOT YOSYS_BIN)
  message(FATAL_ERROR "Yosys was not found.")
endif()
message(STATUS "Found Yosys binary = ${YOSYS_BIN}")

# Compiler
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -pedantic)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(sim sim.cpp)
target_link_libraries(sim ${Boost_LIBRARIES})

add_executable(asm asm.cpp)
target_link_libraries(asm ${Boost_LIBRARIES})

install(TARGETS asm sim DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(verilog)

enable_testing()
add_subdirectory(tests)
