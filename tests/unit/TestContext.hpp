#ifndef TEST_CONTEXT_HPP
#define TEST_CONTEXT_HPP

#include "hexasm.hpp"
#include "hexsim.hpp"
#include "xcmp.hpp"

struct TestContext {
  TestContext() {}

  /// Convert an assembly program into tokens.
  std::string tokeniseAsmProgram(const std::string &inBuffer) {
    hexasm::Lexer lexer;
    std::ostringstream outBuffer;
    lexer.loadBuffer(inBuffer);
    lexer.emitTokens(outBuffer);
    return outBuffer.str();
  }

  /// Parse and emit the tree of an assembly program into an output buffer.
  std::string treeAsmProgram(const std::string &inBuffer) {
    hexasm::Lexer lexer;
    hexasm::Parser parser(lexer);
    lexer.loadBuffer(inBuffer);
    auto program = parser.parseProgram();
    hexasm::prepareProgram(program);
    std::ostringstream outBuffer;
    hexasm::emitProgramText(program, outBuffer);
    return outBuffer.str();
  }

  /// Assemble an assembly program (as binary data) into an output buffer.
  std::ostringstream assembleAsmProgram(const std::string &inBuffer) {
    hexasm::Lexer lexer;
    hexasm::Parser parser(lexer);
    lexer.loadBuffer(inBuffer);
    auto program = parser.parseProgram();
    hexasm::prepareProgram(program);
    std::ostringstream outBuffer;
    hexasm::emitProgramBin(program, outBuffer);
    return outBuffer;
  }
};

#endif // TEST_CONTEXT_HPP
