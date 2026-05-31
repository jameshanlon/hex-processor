#include <cstring>
#include <iostream>

#include "hexdis.hpp"

//===---------------------------------------------------------------------===//
// Driver
//===---------------------------------------------------------------------===//

static void help(const char *argv[]) {
  std::cout << "Hex disassembler\n\n";
  std::cout << "Usage: " << argv[0] << " file\n\n";
  std::cout << "Positional arguments:\n";
  std::cout << "  file              A binary file to disassemble\n\n";
  std::cout << "Optional arguments:\n";
  std::cout << "  -h,--help         Display this message\n";
  std::cout << "  --no-labels       Don't display debug labels\n";
}

/// Read and disassemble a hex binary file.
int main(int argc, const char *argv[]) {
  try {
    const char *filename = nullptr;
    bool showLabels = true;
    for (int i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0) {
        help(argv);
        return 0;
      } else if (std::strcmp(argv[i], "--no-labels") == 0) {
        showLabels = false;
      } else if (argv[i][0] == '-') {
        throw std::runtime_error(std::string("unrecognised argument: ") +
                                 argv[i]);
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

    // Read binary.
    std::vector<uint8_t> program;
    hexdis::DebugInfo debugInfo;
    hexdis::readBinaryFile(filename, program, debugInfo);

    // Disassemble.
    hexdis::disassemble(program, std::cout, debugInfo, showLabels);

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
