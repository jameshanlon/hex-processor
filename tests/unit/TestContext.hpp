#ifndef TEST_CONTEXT_HPP
#define TEST_CONTEXT_HPP

#include "hexasm.hpp"
#include "hexsim.hpp"
#include "xcmp.hpp"

struct TestContext {
  TestContext() {}

  /// Convert an assembly program into tokens.
  std::ostringstream tokHexProgram(const std::string &inBuffer) {
    hexasm::Lexer lexer;
    std::ostringstream outBuffer;
    lexer.loadBuffer(inBuffer);
    lexer.emitTokens(outBuffer);
    return outBuffer;
  }

  /// Parse and emit the tree of an assembly program into an output buffer.
  std::ostringstream asmHexProgram(const std::string &inBuffer, bool text=false) {
    hexasm::Lexer lexer;
    hexasm::Parser parser(lexer);
    lexer.loadBuffer(inBuffer);
    auto program = parser.parseProgram();
    auto codeGen = hexasm::CodeGen(program);
    std::ostringstream outBuffer;
    if (text) {
      codeGen.emitProgramText(outBuffer);
    } else {
      codeGen.emitProgramBin(outBuffer);
    }
    return outBuffer;
  }

  /// Run an assembly program.
  void runHexProgram(const std::string &inBuffer) {
    // Assemble the program.
    hexasm::Lexer lexer;
    hexasm::Parser parser(lexer);
    lexer.loadBuffer(inBuffer);
    auto program = parser.parseProgram();
    auto codeGen = hexasm::CodeGen(program);
    codeGen.emitBin("a.bin");
    // Run the program.
    std::istringstream simInBuffer;
    std::ostringstream simOutBuffer;
    hexsim::Processor p(simInBuffer, simOutBuffer);
    p.load("a.bin");
    p.run();
  }

  /// Convert an X program into tokens.
  std::ostringstream tokeniseXProgram(const std::string &inBuffer) {
    xcmp::Lexer lexer;
    std::ostringstream outBuffer;
    lexer.loadBuffer(inBuffer);
    lexer.emitTokens(outBuffer);
    return outBuffer;
  }

  /// Parse and emit the AST of an X program into an output buffer.
  std::ostringstream treeXProgram(const std::string &inBuffer) {
    xcmp::Lexer lexer;
    xcmp::Parser parser(lexer);
    std::ostringstream outBuffer;
    xcmp::AstPrinter printer(outBuffer);
    lexer.loadBuffer(inBuffer);
    auto tree = parser.parseProgram();
    tree->accept(&printer);
    return outBuffer;
  }

  /// Parse and emit the assembly of an X program into an output buffer.
  std::ostringstream asmXProgram(const std::string &inBuffer, bool text=false) {
    xcmp::Lexer lexer;
    xcmp::Parser parser(lexer);
    std::ostringstream outBuffer;
    lexer.loadBuffer(inBuffer);
    auto tree = parser.parseProgram();
    xcmp::CodeGen xCodeGen;
    tree->accept(&xCodeGen);
    hexasm::CodeGen hexCodeGen(xCodeGen.getInstrs());
    if (text) {
      hexCodeGen.emitProgramText(outBuffer);
    } else {
      hexCodeGen.emitProgramBin(outBuffer);
    }
    return outBuffer;
  }

  /// Run an X program.
  void runXProgram(const std::string &inBuffer) {
    // Compile and assemble the program.
    xcmp::Lexer lexer;
    xcmp::Parser parser(lexer);
    lexer.loadBuffer(inBuffer);
    auto tree = parser.parseProgram();
    xcmp::CodeGen xCodeGen;
    tree->accept(&xCodeGen);
    // Assemble.
    hexasm::CodeGen hexCodeGen(xCodeGen.getInstrs());
    hexCodeGen.emitBin("a.bin");
    // Run the program.
    std::istringstream simInBuffer;
    std::ostringstream simOutBuffer;
    hexsim::Processor p(simInBuffer, simOutBuffer);
    p.load("a.bin");
    p.run();
  }

};

#endif // TEST_CONTEXT_HPP
