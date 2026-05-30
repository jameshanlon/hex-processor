cmake_minimum_required(VERSION 3.28.0)

project(hex-processor)

include(GNUInstallDirs)
include(FetchContent)

# FetchContent for fmt
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 11.0.2
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(fmt)

# FetchContent for Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(Catch2)

# Python
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Use local find scripts
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

option(USE_VERILATOR "Use Verilator for simulation" ON)
option(BUILD_DOCS "Create and install HTML documentation" OFF)

# Verilator. Built from source via FetchContent so the toolchain version is
# pinned and self-contained. Requires host build tools: autoconf, flex, bison.
if(USE_VERILATOR)
  set(VERILATOR_GIT_TAG "v5.048" CACHE STRING "Verilator version to fetch/build")
  set(VERILATOR_INSTALL_DIR "${CMAKE_BINARY_DIR}/verilator")
  set(VERILATOR_ROOT "${VERILATOR_INSTALL_DIR}/share/verilator")

  # Build Verilator once; subsequent configures reuse the installed tree.
  if(NOT EXISTS "${VERILATOR_ROOT}/verilator-config.cmake")
    message(STATUS "Fetching and building Verilator ${VERILATOR_GIT_TAG} "
                   "(one-time, needs autoconf/flex/bison)...")
    FetchContent_Declare(
      verilator
      GIT_REPOSITORY https://github.com/verilator/verilator.git
      GIT_TAG ${VERILATOR_GIT_TAG}
      GIT_SHALLOW TRUE)
    FetchContent_GetProperties(verilator)
    if(NOT verilator_POPULATED)
      FetchContent_Populate(verilator)
    endif()

    include(ProcessorCount)
    ProcessorCount(NPROC)
    if(NPROC EQUAL 0)
      set(NPROC 1)
    endif()

    # Verilator uses an autoconf build; a git checkout has no generated
    # configure, so run autoconf first.
    execute_process(COMMAND autoconf
                    WORKING_DIRECTORY ${verilator_SOURCE_DIR}
                    RESULT_VARIABLE VL_AUTOCONF)
    if(VL_AUTOCONF)
      message(FATAL_ERROR
        "Could not run 'autoconf' to build Verilator. Install the host build "
        "tools first: autoconf, flex, bison (e.g. "
        "'apt-get install autoconf flex bison'), then reconfigure.")
    endif()
    execute_process(COMMAND ${verilator_SOURCE_DIR}/configure
                            --prefix=${VERILATOR_INSTALL_DIR}
                    WORKING_DIRECTORY ${verilator_SOURCE_DIR}
                    RESULT_VARIABLE VL_CONFIGURE)
    execute_process(COMMAND make -j${NPROC}
                    WORKING_DIRECTORY ${verilator_SOURCE_DIR}
                    RESULT_VARIABLE VL_MAKE)
    execute_process(COMMAND make install
                    WORKING_DIRECTORY ${verilator_SOURCE_DIR}
                    RESULT_VARIABLE VL_INSTALL)
    if(VL_CONFIGURE OR VL_MAKE OR VL_INSTALL)
      message(FATAL_ERROR
        "Verilator build failed. Ensure autoconf, flex and bison are installed.")
    endif()
  endif()

  find_package(verilator REQUIRED HINTS ${VERILATOR_ROOT})
  message(STATUS "Using Verilator ${VERILATOR_GIT_TAG} from ${VERILATOR_ROOT}")
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

# Disassembler
add_executable(hexdis hex.cpp hexdis.cpp)
target_link_libraries(hexdis fmt::fmt)

# Compile and run
add_executable(xrun hex.cpp xrun.cpp)
target_link_libraries(xrun fmt::fmt)

install(TARGETS hexasm xcmp xrun hexsim hexdis
        DESTINATION ${CMAKE_INSTALL_BINDIR})

# Verilator
if(USE_VERILATOR)
  add_executable(hextb hex.cpp hextb.cpp)
  target_link_libraries(hextb PUBLIC fmt::fmt)

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
