configure_file(definitions.py.in
               definitions.py)

configure_file(tests.py ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

add_test(NAME tests
         COMMAND ${Python3_EXECUTABLE} tests.py)

add_subdirectory(unit)