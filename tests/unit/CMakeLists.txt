configure_file(definitions.hpp.in
               definitions.hpp)

add_executable(UnitTests
               main.cpp
               ../../hex.cpp
               asm_tests.cpp
               x_features.cpp
               x_programs.cpp)

target_link_libraries(UnitTests
                      ${Boost_LIBRARIES})

target_include_directories(UnitTests PUBLIC
                           ${CMAKE_SOURCE_DIR}
                           ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME UnitTests COMMAND ./UnitTests)