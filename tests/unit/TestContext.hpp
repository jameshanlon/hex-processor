#ifndef TEST_CONTEXT_HPP
#define TEST_CONTEXT_HPP

#include "definitions.hpp"
#include "hexasm.hpp"
#include "hexsim.hpp"
#include "xcmp.hpp"

struct TestContext {
  std::istringstream simInBuffer;
  std::ostringstream simOutBuffer;

  TestContext() {}

  /// Return the path to a test filename.
  std::string getAsmTestPath(std::string filename) {
    boost::filesystem::path testPath(ASM_TEST_SRC_PREFIX);
    testPath /= filename;
    return testPath.string();
  }

  /// Return the path to a test filename.
  std::string getXTestPath(std::string filename) {
    boost::filesystem::path testPath(X_TEST_SRC_PREFIX);
    testPath /= filename;
    return testPath.string();
  }

  /// TODO: merge these routines into a single one based on an action.
  /// Convert an assembly program into tokens.
  std::ostringstream tokHexProgram(const std::string &program,
                                   bool isFilename=false) {
    hexasm::Lexer lexer;
    if (isFilename) {
      lexer.openFile(program);
    } else {
      lexer.loadBuffer(program);
    }
    std::ostringstream outBuffer;
    lexer.emitTokens(outBuffer);
    return outBuffer;
  }

  /// Parse and emit the tree of an assembly program into an output buffer.
  std::ostringstream asmHexProgram(const std::string &program,
                                   bool isFilename=false,
                                   bool emitText=false) {
    hexasm::Lexer lexer;
    hexasm::Parser parser(lexer);
    if (isFilename) {
      lexer.openFile(program);
    } else {
      lexer.loadBuffer(program);
    }
    auto tree = parser.parseProgram();
    auto codeGen = hexasm::CodeGen(tree);
    std::ostringstream outBuffer;
    if (emitText) {
      codeGen.emitProgramText(outBuffer);
    } else {
      codeGen.emitProgramBin(outBuffer);
    }
    return outBuffer;
  }

  /// Run an assembly program.
  int runHexProgram(const std::string &program,
                    bool isFilename=false) {
    // Assemble the program.
    hexasm::Lexer lexer;
    hexasm::Parser parser(lexer);
    if (isFilename) {
      lexer.openFile(program);
    } else {
      lexer.loadBuffer(program);
    }
    auto tree = parser.parseProgram();
    auto codeGen = hexasm::CodeGen(tree);
    codeGen.emitBin("a.bin");
    // Run the program.
    hexsim::Processor p(simInBuffer, simOutBuffer);
    p.load("a.bin");
    return p.run();
  }

  /// Convert an X program into tokens.
  std::ostringstream tokeniseXProgram(const std::string &program,
                                      bool isFilename=false) {
    xcmp::Lexer lexer;
    std::ostringstream outBuffer;
    if (isFilename) {
      lexer.openFile(program);
    } else {
      lexer.loadBuffer(program);
    }
    lexer.emitTokens(outBuffer);
    return outBuffer;
  }

  /// Parse and emit the AST of an X program into an output buffer.
  std::ostringstream treeXProgram(const std::string &program,
                                  bool isFilename=false) {
    xcmp::Lexer lexer;
    xcmp::Parser parser(lexer);
    std::ostringstream outBuffer;
    xcmp::AstPrinter printer(outBuffer);
    if (isFilename) {
      lexer.openFile(program);
    } else {
      lexer.loadBuffer(program);
    }
    auto tree = parser.parseProgram();
    tree->accept(&printer);
    return outBuffer;
  }

  /// Parse and emit the assembly of an X program into an output buffer.
  std::ostringstream asmXProgram(const std::string &program,
                                 bool isFilename=false,
                                 bool text=false) {
    xcmp::Lexer lexer;
    xcmp::Parser parser(lexer);
    if (isFilename) {
      lexer.openFile(program);
    } else {
      lexer.loadBuffer(program);
    }
    auto tree = parser.parseProgram();
    xcmp::SymbolTable symbolTable;
    xcmp::ConstProp constProp(symbolTable);
    tree->accept(&constProp);
    xcmp::CodeGen xCodeGen(symbolTable);
    tree->accept(&xCodeGen);
    xcmp::LowerDirectives lowerDirectives(symbolTable, xCodeGen);
    hexasm::CodeGen hexCodeGen(lowerDirectives.getInstrs());
    std::ostringstream outBuffer;
    if (text) {
      hexCodeGen.emitProgramText(outBuffer);
    } else {
      hexCodeGen.emitProgramBin(outBuffer);
    }
    return outBuffer;
  }

  /// Run an X program.
  int runXProgram(const std::string &program,
                  bool isFilename=false) {
    // Compile and assemble the program.
    xcmp::Lexer lexer;
    xcmp::Parser parser(lexer);
    if (isFilename) {
      lexer.openFile(program);
    } else {
      lexer.loadBuffer(program);
    }
    auto tree = parser.parseProgram();
    xcmp::SymbolTable symbolTable;
    xcmp::ConstProp constProp(symbolTable);
    tree->accept(&constProp);
    xcmp::CodeGen xCodeGen(symbolTable);
    tree->accept(&xCodeGen);
    xcmp::LowerDirectives lowerDirectives(symbolTable, xCodeGen);
    // Assemble.
    hexasm::CodeGen hexCodeGen(lowerDirectives.getInstrs());
    hexCodeGen.emitBin("a.bin");
    // Run the program.
    hexsim::Processor p(simInBuffer, simOutBuffer);
    p.load("a.bin");
    return p.run();
  }

};

#endif // TEST_CONTEXT_HPP
