#include <iostream>

#include "hexasm.hpp"
#include "xcmp.hpp"
#include "hexsim.hpp"
#include "hexsimio.hpp"

//===---------------------------------------------------------------------===//
// Driver
//===---------------------------------------------------------------------===//

static void help(char *argv[]) {
  std::cout << "X run\n\n";
  std::cout << "Usage: " << argv[0] << " file\n\n";
  std::cout << "Positional arguments:\n";
  std::cout << "  file              A source file to run\n\n";
  std::cout << "Optional arguments:\n";
  std::cout << "  -h,--help         Display this message\n";
  std::cout << "  -t,--trace      Enable instruction tracing\n";
  std::cout << "  --max-cycles N  Limit the number of simulation cycles (default: 0)\n";
}

int main(int argc, char *argv[]) {
  char *inputFilename = nullptr;
  bool trace = false;
  size_t maxCycles = 0;
  xcmp::Driver driver(std::cout);
  try {
    for (int i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0) {
        help(argv);
        std::exit(1);
      } else if (std::strcmp(argv[i], "-t") == 0 ||
                 std::strcmp(argv[i], "--trace") == 0) {
        trace = true;
      } else if (std::strcmp(argv[i], "--max-cycles") == 0) {
        maxCycles = std::stoull(argv[++i]);
      } else if (argv[i][0] == '-') {
          throw std::runtime_error(std::string("unrecognised argument: ")+argv[i]);
      } else {
        if (!inputFilename) {
          inputFilename = argv[i];
        } else {
          throw std::runtime_error("cannot specify more than one file");
        }
      }
    }
    driver.runCatchExceptions(xcmp::DriverAction::EMIT_BINARY, inputFilename, true, "a.bin", false);
    hexsim::Processor processor(std::cin, std::cout, maxCycles);
    processor.setTracing(trace);
    processor.load("a.bin");
    processor.run();
  } catch (const std::exception &e) {
    std::cerr << boost::format("Error: %s\n") % e.what();
    return 1;
  }
  return 0;
}