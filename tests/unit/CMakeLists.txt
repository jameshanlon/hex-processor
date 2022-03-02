add_executable(UnitTests
               main.cpp
               hexasm.cpp)

target_link_libraries(UnitTests
                      ${Boost_LIBRARIES})

target_include_directories(UnitTests PUBLIC
                           ${CMAKE_SOURCE_DIR}
                           ${CMAKE_BINARY_DIR})

add_test(NAME UnitTests COMMAND ./UnitTests)