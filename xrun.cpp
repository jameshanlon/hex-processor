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
}

int main(int argc, char *argv[]) {
  char *inputFilename;
  std::istringstream simInBuffer;
  std::ostringstream simOutBuffer;
  xcmp::Driver driver(std::cout);
  try {
    for (int i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0) {
        help(argv);
        std::exit(1);
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
    driver.runCatchExceptions(xcmp::DriverAction::EMIT_BINARY, inputFilename, true, "a.bin");
    hexsim::Processor processor(simInBuffer, simOutBuffer);
    processor.load("a.bin");
    processor.run();
    std::cout << simOutBuffer.str();
  } catch (const std::exception &e) {
    std::cerr << boost::format("Error: %s\n") % e.what();
    return 1;
  }
  return 0;
}