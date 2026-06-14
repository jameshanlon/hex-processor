# Look for an executable called sphinx-build. Prefer the project-local
# virtualenv (docs/_venv, created from docs/requirements.txt) if present, so the
# pinned Sphinx is used; otherwise fall back to one on PATH.
find_program(
  SPHINX_EXECUTABLE
  NAMES sphinx-build
  HINTS ${CMAKE_SOURCE_DIR}/docs/_venv/bin
  DOC "Path to sphinx-build executable")

include(FindPackageHandleStandardArgs)

# Handle standard arguments to find_package like REQUIRED and QUIET
find_package_handle_standard_args(
  Sphinx "Failed to find sphinx-build executable" SPHINX_EXECUTABLE)
