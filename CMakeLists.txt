cmake_minimum_required(VERSION 3.21.0)
project(hex-processor)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.68.0 REQUIRED COMPONENTS
             unit_test_framework)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(sim sim.cpp)
target_compile_options(sim PUBLIC -Wall -pedantic)
target_link_libraries(sim
                      ${Boost_LIBRARIES})
