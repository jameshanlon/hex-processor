cmake_minimum_required(VERSION 3.20...3.28)

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

# Verilator. Prefer a sufficiently recent system install; otherwise fetch and
# build a pinned version from source (needs host tools: autoconf, flex, bison).
if(USE_VERILATOR)
  set(VERILATOR_GIT_TAG
      "v5.048"
      CACHE
        STRING
        "Verilator version to fetch/build when no system install is suitable")
  set(VERILATOR_MIN_VERSION
      "5.0"
      CACHE STRING "Minimum acceptable system Verilator version")

  # First try a system install (honouring VERILATOR_ROOT). The version floor
  # rejects too-old installs (e.g. 4.x lacks the VerilatedContext API hextb
  # uses), so the source build below takes over.
  find_package(verilator ${VERILATOR_MIN_VERSION} QUIET HINTS
               $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT})

  if(verilator_FOUND)
    message(
      STATUS
        "Using system Verilator ${verilator_VERSION} (>= ${VERILATOR_MIN_VERSION})"
    )
  else()
    set(VERILATOR_INSTALL_DIR "${CMAKE_BINARY_DIR}/verilator")
    set(VERILATOR_ROOT "${VERILATOR_INSTALL_DIR}/share/verilator")

    # Build once; subsequent configures reuse the installed tree.
    if(NOT EXISTS "${VERILATOR_ROOT}/verilator-config.cmake")
      message(STATUS "No system Verilator >= ${VERILATOR_MIN_VERSION}; "
                     "fetching and building ${VERILATOR_GIT_TAG} "
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
      execute_process(
        COMMAND autoconf
        WORKING_DIRECTORY ${verilator_SOURCE_DIR}
        RESULT_VARIABLE VL_AUTOCONF)
      if(VL_AUTOCONF)
        message(
          FATAL_ERROR
            "Could not run 'autoconf' to build Verilator. Install the host build "
            "tools first: autoconf, flex, bison (e.g. "
            "'apt-get install autoconf flex bison'), then reconfigure.")
      endif()
      execute_process(
        COMMAND ${verilator_SOURCE_DIR}/configure
                --prefix=${VERILATOR_INSTALL_DIR}
        WORKING_DIRECTORY ${verilator_SOURCE_DIR}
        RESULT_VARIABLE VL_CONFIGURE)
      if(VL_CONFIGURE)
        message(
          FATAL_ERROR "Verilator ./configure failed (need flex and bison).")
      endif()
      # Build only the binaries (verilator_exe) and install without man pages,
      # so help2man is not required.
      execute_process(
        COMMAND make -j${NPROC} verilator_exe
        WORKING_DIRECTORY ${verilator_SOURCE_DIR}
        RESULT_VARIABLE VL_MAKE)
      if(VL_MAKE)
        message(FATAL_ERROR "Verilator build (make) failed.")
      endif()
      execute_process(
        COMMAND make installbin installredirect installdata install-msg
        WORKING_DIRECTORY ${verilator_SOURCE_DIR}
        RESULT_VARIABLE VL_INSTALL)
      if(VL_INSTALL)
        message(FATAL_ERROR "Verilator install failed.")
      endif()
    endif()

    # Clear any cached binary (e.g. an earlier system-Verilator probe) so the
    # config resolves verilator_bin against the fetched install.
    unset(VERILATOR_BIN CACHE)
    find_package(verilator REQUIRED HINTS ${VERILATOR_ROOT})
    message(
      STATUS
        "Using fetched Verilator ${VERILATOR_GIT_TAG} from ${VERILATOR_ROOT}")
  endif()
endif()

# Compiler config
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -pedantic)

# Library headers (header-only modules + shared hex.cpp) live in src/.
include_directories(${PROJECT_SOURCE_DIR}/src)

# Shared library code, compiled once and linked into every tool and the tests.
add_library(hexcommon STATIC src/hex.cpp)

# Simulator
add_executable(hexsim tools/hexsim.cpp)
target_link_libraries(hexsim hexcommon fmt::fmt)

# Assembler
add_executable(hexasm tools/hexasm.cpp)
target_link_libraries(hexasm hexcommon fmt::fmt)

# Compiler
add_executable(xcmp tools/xcmp.cpp)
target_link_libraries(xcmp hexcommon fmt::fmt)

# Disassembler
add_executable(hexdis tools/hexdis.cpp)
target_link_libraries(hexdis hexcommon fmt::fmt)

# Compile and run
add_executable(xrun tools/xrun.cpp)
target_link_libraries(xrun hexcommon fmt::fmt)

install(TARGETS hexasm xcmp xrun hexsim hexdis
        DESTINATION ${CMAKE_INSTALL_BINDIR})

# Verilator
if(USE_VERILATOR)
  add_executable(hextb tools/hextb.cpp)
  target_link_libraries(hextb PUBLIC hexcommon fmt::fmt)

  verilate(
    hextb
    PREFIX
    Vntb
    TOP_MODULE
    network_top
    VERILATOR_ARGS
    -Wall
    -Wno-UNUSEDPARAM
    -Wno-UNUSEDSIGNAL
    SOURCES
    rtl/hex_pkg.sv
    rtl/memory.sv
    rtl/processor.sv
    rtl/link_interface.sv
    rtl/core.sv
    rtl/router.sv
    rtl/network_top.sv)

  install(TARGETS hextb DESTINATION ${CMAKE_INSTALL_BINDIR})

  add_executable(router_tb tests/rtl/router_tb.cpp)
  verilate(
    router_tb
    PREFIX
    Vrouter
    SOURCES
    rtl/hex_pkg.sv
    rtl/router.sv
    TOP_MODULE
    router
    VERILATOR_ARGS
    -Wall
    -Wno-UNUSEDPARAM)

  add_executable(liu_tb tests/rtl/liu_tb.cpp)
  verilate(
    liu_tb
    SOURCES
    rtl/hex_pkg.sv
    rtl/link_interface.sv
    PREFIX
    Vlink_interface
    TOP_MODULE
    link_interface
    VERILATOR_ARGS
    -Wall
    -Wno-UNUSEDPARAM
    -Wno-UNUSEDSIGNAL)

  add_executable(core_tb tests/rtl/core_tb.cpp)
  verilate(
    core_tb
    SOURCES
    rtl/hex_pkg.sv
    rtl/memory.sv
    rtl/processor.sv
    rtl/link_interface.sv
    rtl/core.sv
    PREFIX
    Vcore
    TOP_MODULE
    core
    VERILATOR_ARGS
    -Wall
    -Wno-UNUSEDPARAM
    -Wno-UNUSEDSIGNAL)
endif()

enable_testing()
add_subdirectory(tests)

if(BUILD_DOCS)
  add_subdirectory(docs)
endif()
