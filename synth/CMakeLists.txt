# Yosys
if (USE_YOSYS)
  configure_file(synth.in.ys
                 synth.ys)

  add_custom_target(processor.netlist.v ALL
                    COMMAND ${YOSYS_BIN} synth.ys
                    COMMENT "Running Yosys"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    SOURCES processor.v)
endif()

