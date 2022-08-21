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

  /// Return some interesting char values for testing operators.
  const std::vector<char> getCharValues() {
    return {-128, -10, -3, -2, -1, 0, 1, 2, 3, 10, 127};
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
                    bool isFilename=false,
                    bool trace=false) {
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
    hexsim::Processor processor(simInBuffer, simOutBuffer);
    processor.load("a.bin");
    processor.setTracing(trace);
    processor.setTruncateInputs(false);
    return processor.run();
  }

  /// Convert an X program into tokens.
  std::ostringstream tokeniseXProgram(const std::string &program,
                                      bool isFilename=false) {
    std::ostringstream outBuffer;
    xcmp::Driver driver(outBuffer);
    driver.run(xcmp::DriverAction::EMIT_TOKENS, program, isFilename);
    return outBuffer;
  }

  /// Parse and emit the AST of an X program into an output buffer.
  std::ostringstream treeXProgram(const std::string &program,
                                  bool isFilename=false) {
    std::ostringstream outBuffer;
    xcmp::Driver driver(outBuffer);
    driver.run(xcmp::DriverAction::EMIT_TREE, program, isFilename);
    return outBuffer;
  }

  /// Parse and emit the assembly of an X program into an output buffer.
  std::ostringstream asmXProgram(const std::string &program,
                                 bool isFilename=false,
                                 bool text=false) {
    std::ostringstream outBuffer;
    xcmp::Driver driver(outBuffer);
    if (text) {
      driver.run(xcmp::DriverAction::EMIT_ASM, program, isFilename);
    } else {
      driver.run(xcmp::DriverAction::EMIT_BINARY, program, isFilename);
    }
    return outBuffer;
  }

  /// Run an X program.
  int runXProgramSrc(const std::string &program,
                     const std::string input="",
                     bool trace=false) {
    // Compile and assemble the program.
    xcmp::Driver driver(std::cout);
    driver.run(xcmp::DriverAction::EMIT_BINARY, program, false, "a.bin");
    // Run the program.
    simInBuffer.clear();
    simInBuffer.str(input);
    hexsim::Processor processor(simInBuffer, simOutBuffer);
    processor.load("a.bin");
    processor.setTracing(trace);
    processor.setTruncateInputs(false);
    return processor.run();
  }

  int runXProgramFile(const std::string &filename,
                      const std::string input="",
                      bool trace=false) {
    // Compile and assemble the program.
    xcmp::Driver driver(std::cout);
    driver.run(xcmp::DriverAction::EMIT_BINARY, filename, true, "a.bin");
    // Run the program.
    simInBuffer.clear();
    simInBuffer.str(input);
    hexsim::Processor processor(simInBuffer, simOutBuffer);
    processor.load("a.bin");
    processor.setTracing(trace);
    processor.setTruncateInputs(false);
    return processor.run();
  }
};

#endif // TEST_CONTEXT_HPP
