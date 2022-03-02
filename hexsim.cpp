#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>

#include "hexsim.hpp"
#include "util.hpp"

//===---------------------------------------------------------------------===//
// Driver
//===---------------------------------------------------------------------===//

static void help(const char *argv[]) {
  std::cout << "Hex processor simulator\n\n";
  std::cout << "Usage: " << argv[0] << " file\n\n";
  std::cout << "Positional arguments:\n";
  std::cout << "  file A binary file to simulate\n\n";
  std::cout << "Optional arguments:\n";
  std::cout << "  -h,--help  Display this message\n";
  std::cout << "  -d,--dump  Dump the binary file contents\n";
  std::cout << "  -t,--trace Enable instruction tracing\n";
}

int main(int argc, const char *argv[]) {
  try {
    const char *filename = nullptr;
    bool dumpBinary = false;
    bool trace = false;
    for (unsigned i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-d") == 0 ||
          std::strcmp(argv[i], "--dump") == 0) {
        dumpBinary = true;
      } else if (std::strcmp(argv[i], "-t") == 0 ||
                 std::strcmp(argv[i], "--trace") == 0) {
        trace = true;
      } else if (std::strcmp(argv[i], "-h") == 0 ||
                 std::strcmp(argv[i], "--help") == 0) {
        help(argv);
        return 1;
      } else {
        if (!filename) {
          filename = argv[i];
        } else {
          throw std::runtime_error("cannot specify more than one file");
        }
      }
    }
    // A file must be specified.
    if (!filename) {
      help(argv);
      return 1;
    }
    hexsim::Processor p;
    p.setTracing(trace);
    p.load(filename, dumpBinary);
    if (dumpBinary) {
      return 0;
    }
    p.run();
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
