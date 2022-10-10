#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <boost/format.hpp>

#include "hex.hpp"
#include "hexasm.hpp"
#include "xcmp.hpp"

//===---------------------------------------------------------------------===//
// Driver
//===---------------------------------------------------------------------===//

static void help(const char *argv[]) {
  std::cout << "X compiler\n\n";
  std::cout << "Usage: " << argv[0] << " file\n\n";
  std::cout << "Positional arguments:\n";
  std::cout << "  file              A source file to compile\n\n";
  std::cout << "Optional arguments:\n";
  std::cout << "  -h,--help         Display this message\n";
  std::cout << "  --tokens          Tokenise the input only\n";
  std::cout << "  --tree            Display the syntax tree only\n";
  std::cout << "  --tree-opt        Display the optimised syntax tree only\n";
  std::cout << "  --insts           Display the intermediate instructions only\n";
  std::cout << "  --insts-lowered   Display the lowered instructions only\n";
  std::cout << "  --insts-optimised Display the lowered optimised instructions only\n";
  std::cout << "  --memory-info     Report memory information\n";
  std::cout << "  -S                Emit the assembly program\n";
  std::cout << "  --insts-asm       Display the assembled instructions only\n";
  std::cout << "  -o,--output file  Specify a file for output (default a.out)\n";
}

int main(int argc, const char *argv[]) {
  xcmp::Driver driver(std::cout);

  try {
    // Handle arguments.
    auto driverAction = xcmp::DriverAction::EMIT_BINARY;
    const char *inputFilename = nullptr;
    const char *outputFilename = "a.out";
    bool reportMemoryInfo = false;
    for (int i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0) {
        help(argv);
        std::exit(1);
      } else if (std::strcmp(argv[i], "--tokens") == 0) {
        driverAction = xcmp::DriverAction::EMIT_TOKENS;
      } else if (std::strcmp(argv[i], "--tree") == 0) {
        driverAction = xcmp::DriverAction::EMIT_TREE;
      } else if (std::strcmp(argv[i], "--tree-opt") == 0) {
        driverAction = xcmp::DriverAction::EMIT_OPTIMISED_TREE;
      } else if (std::strcmp(argv[i], "--insts") == 0) {
        driverAction = xcmp::DriverAction::EMIT_INTERMEDIATE_INSTS;
      } else if (std::strcmp(argv[i], "--insts-lowered") == 0) {
        driverAction = xcmp::DriverAction::EMIT_LOWERED_INSTS;
      } else if (std::strcmp(argv[i], "--insts-optimised") == 0) {
        driverAction = xcmp::DriverAction::EMIT_OPTIMISED_INSTS;
      } else if (std::strcmp(argv[i], "-S") == 0) {
        driverAction = xcmp::DriverAction::EMIT_ASM;
      } else if (std::strcmp(argv[i], "--insts-asm") == 0) {
        driverAction = xcmp::DriverAction::EMIT_TREE;
      } else if (std::strcmp(argv[i], "--memory-info") == 0) {
        reportMemoryInfo = true;
      } else if (std::strcmp(argv[i], "--output") == 0 ||
                 std::strcmp(argv[i], "-o") == 0) {
        outputFilename = argv[++i];
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
    // A file must be specified.
    if (!inputFilename) {
      help(argv);
      std::exit(1);
    }
    // Run.
    return driver.runCatchExceptions(driverAction, inputFilename, outputFilename, "a.out", reportMemoryInfo);
  } catch (const std::exception &e) {
    std::cerr << boost::format("Error: %s\n") % e.what();
    return 1;
  }
}
