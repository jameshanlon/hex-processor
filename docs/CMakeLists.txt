find_package(Sphinx REQUIRED)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

configure_file(conf.py ${CMAKE_CURRENT_BINARY_DIR})

# Build strictly: -W turns warnings (orphan pages, broken cross-references) into
# errors and --keep-going reports them all, matching how the docs are verified.
add_custom_target(
  Sphinx ALL
  COMMAND ${SPHINX_EXECUTABLE} -b html -W --keep-going
          -c ${CMAKE_CURRENT_BINARY_DIR} ${SPHINX_SOURCE} ${SPHINX_BUILD}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating documentation with Sphinx")
