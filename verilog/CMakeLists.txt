add_executable(hextb sim_main.cpp)

verilate(hextb
         SOURCES hextb.sv)
