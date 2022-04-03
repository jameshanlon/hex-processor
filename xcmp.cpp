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
  std::cout << "  -S                Display the generated assembly code only\n";
  std::cout << "  -o,--output file  Specify a file for output (default a.S)\n";
}

int main(int argc, const char *argv[]) {
  xcmp::Lexer lexer;
  xcmp::Parser parser(lexer);

  try {

    // Handle arguments.
    bool tokensOnly = false;
    bool treeOnly = false;
    bool asmOnly = false;
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
      } else if (std::strcmp(argv[i], "-S") == 0) {
        asmOnly = true;
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
    auto tree = parser.parseProgram();

    xcmp::SymbolTable symbolTable;

    // Constant propagation.
    xcmp::ConstProp constProp(symbolTable);
    tree->accept(&constProp);

    // Parse and print program only.
    if (treeOnly) {
      xcmp::AstPrinter printer;
      tree->accept(&printer);
      return 0;
    }

    // Perform code generation.
    xcmp::CodeGen codeGen(symbolTable);
    tree->accept(&codeGen);

    // Lower the generated (intermediate code) to assembly directives.
    xcmp::LowerDirectives lowerDirectives(codeGen);

    // Assemble the instructions.
    hexasm::CodeGen asmCodeGen(lowerDirectives.getInstrs());

    // Print the assembly instructions only.
    if (asmOnly) {
      asmCodeGen.emitProgramText(std::cout);
      return 0;
    }

    asmCodeGen.emitBin(outputFilename);

  } catch (const hexutil::Error &e) {
    if (e.hasLocation()) {
      std::cerr << boost::format("Error %s: %s\n") % e.getLocation().str() % e.what();
    } else {
      std::cerr << boost::format("Error: %s\n") % e.what();
    }
    if (lexer.hasLine()) {
      std::cerr << "  " << lexer.getLine() << "\n";
    }
    return 1;
  } catch (const std::exception &e) {
    std::cerr << boost::format("Error: %s\n") % e.what();
    return 1;
  }
  return 0;
}
