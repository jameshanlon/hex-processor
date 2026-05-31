configure_file(definitions.hpp.in definitions.hpp)

add_executable(
  UnitTests
  ../../hex.cpp
  AssemblerTests.cpp
  AssemblerProgramTests.cpp
  XLanguageTests.cpp
  XProgramTests.cpp
  DisassemblerTests.cpp
  SimTests.cpp)

target_link_libraries(UnitTests PRIVATE Catch2::Catch2WithMain fmt::fmt)

target_include_directories(UnitTests PUBLIC ${CMAKE_SOURCE_DIR}
                                            ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME UnitTests COMMAND ./UnitTests)
