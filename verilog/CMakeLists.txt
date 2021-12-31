
# =============================================================================
# Verilator
# =============================================================================

add_executable(hextb hextb.cpp)

verilate(hextb
         SOURCES hex.sv)

# =============================================================================
# Yosys
# =============================================================================

configure_file(synth.in.ys
               synth.ys)

add_custom_target(netlist.v ALL
                  COMMAND ${YOSYS_BIN} synth.ys
                  COMMENT "Running Yosys"
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  SOURCES processor.sv)
