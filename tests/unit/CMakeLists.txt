add_executable(
  UnitTests AssemblerTests.cpp AssemblerProgramTests.cpp XLanguageTests.cpp
            XProgramTests.cpp DisassemblerTests.cpp SimTests.cpp)

target_link_libraries(UnitTests PRIVATE hexcommon Catch2::Catch2WithMain
                                        fmt::fmt)

target_compile_definitions(
  UnitTests
  PRIVATE "ASM_TEST_SRC_PREFIX=\"${CMAKE_SOURCE_DIR}/tests/asm\""
          "X_TEST_SRC_PREFIX=\"${CMAKE_SOURCE_DIR}/examples\""
          "CURRENT_BINARY_DIRECTORY=\"${CMAKE_CURRENT_BINARY_DIR}\"")

target_include_directories(UnitTests PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_test(NAME UnitTests COMMAND ./UnitTests)
