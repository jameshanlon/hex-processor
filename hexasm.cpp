#include "hexasm.hpp"

//===---------------------------------------------------------------------===//
// Driver
//===---------------------------------------------------------------------===//

static void help(const char *argv[]) {
  std::cout << "Hex assembler\n\n";
  std::cout << "Usage: " << argv[0] << " file\n\n";
  std::cout << "Positional arguments:\n";
  std::cout << "  file              A source file to assemble\n\n";
  std::cout << "Optional arguments:\n";
  std::cout << "  -h,--help         Display this message\n";
  std::cout << "  --tokens          Tokenise the input only\n";
  std::cout << "  --instrs          Display the instruction sequence only\n";
  std::cout << "  -o,--output file  Specify a file for binary output (default a.out)\n";
}

int main(int argc, const char *argv[]) {
  hexasm::Lexer lexer;
  hexasm::Parser parser(lexer);

  try {

    // Handle arguments.
    bool tokensOnly = false;
    bool treeOnly = false;
    const char *filename = nullptr;
    const char *outputFilename = "a.out";
    for (unsigned i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0) {
        help(argv);
        std::exit(1);
      } else if (std::strcmp(argv[i], "--tokens") == 0) {
        tokensOnly = true;
      } else if (std::strcmp(argv[i], "--tree") == 0) {
        treeOnly = true;
      } else if (std::strcmp(argv[i], "--output") == 0 ||
                 std::strcmp(argv[i], "-o") == 0) {
        outputFilename = argv[++i];
      } else if (argv[i][0] == '-') {
          throw std::runtime_error(std::string("unrecognised argument: ")+argv[i]);
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
      std::exit(1);
    }

    // Open the file.
    lexer.openFile(filename);

    // Tokenise only.
    if (tokensOnly && !treeOnly) {
      lexer.emitTokens(std::cout);
      return 0;
    }

    // Parse the program.
    auto program = parser.parseProgram();

    // Initialise code generation from the parsed program.
    hexasm::CodeGen codeGen(program);

    // Print program summary only.
    if (treeOnly) {
      codeGen.emitProgramText(std::cout);
      return 0;
    }

    // Emit the binary file.
    codeGen.emitBin(outputFilename);

  } catch (const hexutil::Error &e) {
    if (e.hasLocation()) {
      std::cerr << boost::format("Error %s: %s\n") % e.getLocation().str() % e.what();
    } else {
      std::cerr << boost::format("Error: %s\n") % e.what();
    }
    if (lexer.hasLine()) {
      std::cerr << "  " << lexer.getLine() << "\n";
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
