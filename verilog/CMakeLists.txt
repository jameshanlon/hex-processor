
# =============================================================================
# Verilator
# =============================================================================

if (USE_VERILATOR)
  add_executable(hextb hextb.cpp)

  verilate(hextb TRACE
           VERILATOR_ARGS --top-module hex
           SOURCES hex_pkg.sv hex.sv)

  install(TARGETS hextb
          DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# =============================================================================
# Yosys
# =============================================================================

if (USE_YOSYS)
  configure_file(synth.in.ys
                 synth.ys)

  add_custom_target(netlist.v ALL
                    COMMAND ${YOSYS_BIN} synth.ys
                    COMMENT "Running Yosys"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    SOURCES processor.sv)
endif()
