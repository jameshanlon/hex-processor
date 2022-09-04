#ifndef TEST_CONTEXT_HPP
#define TEST_CONTEXT_HPP

#include <filesystem>

#include "definitions.hpp"
#include "hexasm.hpp"
#include "hexsim.hpp"
#include "xcmp.hpp"

namespace fs = std::filesystem;

struct TestContext {
  std::istringstream simInBuffer;
  std::ostringstream simOutBuffer;

  TestContext() {}

  /// Return some interesting char values for testing operators.
  const std::vector<char> getCharValues() {
    return {-128, -10, -3, -2, -1, 0, 1, 2, 3, 10, 127};
  }

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

  /// Read the contents of a file into a string.
  std::string readFile(const std::string filename) {
    // Read program to be compiled into the input buffer.
    std::ifstream file(filename);
    // Get file size.
    file.seekg(0,std::ios::end);
    auto length = file.tellg();
    file.seekg(0,std::ios::beg);
    // Read the whole file into the buffer.
    std::vector<char> buffer(length);
    file.read(&buffer[0], length);
    // Close the file.
    file.close();
    return std::string(buffer.begin(), buffer.end());
  }

  /// Simulate a hex program binary.
  int simXBinary(const char *filename,
                 const std::string input={},
                 bool trace=false) {
    //// Initialise in/out buffers.
    simInBuffer.str(input);
    simInBuffer.clear();
    simOutBuffer.str("");
    simOutBuffer.clear();
    // Run the program.
    hexsim::Processor processor(simInBuffer, simOutBuffer);
    processor.load(filename);
    processor.setTracing(trace);
    processor.setTruncateInputs(false);
    return processor.run();
  }

  /// Convert an assembly program from string into tokens.
  std::ostringstream tokHexProgramSrc(const std::string program) {
    hexasm::Lexer lexer;
    lexer.loadBuffer(program);
    std::ostringstream outBuffer;
    lexer.emitTokens(outBuffer);
    return outBuffer;
  }

  /// Convert an assembly program from file into tokens.
  std::ostringstream tokHexProgramFile(const std::string filename) {
    return tokHexProgramSrc(readFile(filename));
  }

  /// Parse and emit the tree of an assembly program from string into an output buffer.
  std::ostringstream asmHexProgramSrc(const std::string program,
                                      bool emitText=false) {
    hexasm::Lexer lexer;
    hexasm::Parser parser(lexer);
    lexer.loadBuffer(program);
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

  /// Parse and emit the tree of an assembly program from file into an output buffer.
  std::ostringstream asmHexProgramFile(const std::string filename,
                                       bool emitText=false) {
    return asmHexProgramSrc(readFile(filename), emitText);
  }

  /// Run an assembly program.
  int runHexProgramSrc(const std::string program,
                       const std::string input={},
                       bool trace=false) {
    // Assemble the program.
    hexasm::Lexer lexer;
    hexasm::Parser parser(lexer);
    lexer.loadBuffer(program);
    fs::path path(CURRENT_BINARY_DIRECTORY);
    path /= fs::path("a.bin").stem();
    auto tree = parser.parseProgram();
    auto codeGen = hexasm::CodeGen(tree);
    codeGen.emitBin(path.c_str());
    // Simulate
    return simXBinary(path.c_str(), input, trace);
  }

  /// Run an assembly program.
  int runHexProgramFile(const std::string filename,
                        const std::string input={},
                        bool trace=false) {
    return runHexProgramSrc(readFile(filename), input, trace);
  }

  /// Convert an X program from string into tokens.
  std::ostringstream tokeniseXProgramSrc(const std::string program) {
    std::ostringstream outBuffer;
    xcmp::Driver driver(outBuffer);
    driver.run(xcmp::DriverAction::EMIT_TOKENS, program, false);
    return outBuffer;
  }

  /// Convert an X program from file into tokens.
  std::ostringstream tokeniseXProgramFile(const std::string filename) {
    return tokeniseXProgramSrc(readFile(filename));
  }

  /// Parse and emit the AST of an X program from string into an output buffer.
  std::ostringstream treeXProgramSrc(const std::string &program) {
    std::ostringstream outBuffer;
    xcmp::Driver driver(outBuffer);
    driver.run(xcmp::DriverAction::EMIT_TREE, program, false);
    return outBuffer;
  }

  /// Parse and emit the AST of an X program from file into an output buffer.
  std::ostringstream treeXProgramFile(const std::string filename) {
    return treeXProgramSrc(readFile(filename));
  }

  /// Parse and emit the assembly of an X program into an output buffer.
  std::ostringstream asmXProgramSrc(const std::string program,
                                    bool text=false) {
    std::ostringstream outBuffer;
    xcmp::Driver driver(outBuffer);
    if (text) {
      driver.run(xcmp::DriverAction::EMIT_ASM, program, false);
    } else {
      driver.run(xcmp::DriverAction::EMIT_BINARY, program, false);
    }
    return outBuffer;
  }

  /// Parse and emit the assembly of an X program into an output buffer.
  std::ostringstream asmXProgramFile(const std::string &filename,
                                     bool text=false) {
    return asmXProgramSrc(readFile(filename), text);
  }

  /// Run an X program from a string.
  int runXProgramSrc(const std::string program,
                     const std::string input={},
                     bool trace=false) {
    // Compile and assemble the program.
    xcmp::Driver driver(std::cout);
    fs::path path(CURRENT_BINARY_DIRECTORY);
    path /= fs::path("a.bin");
    driver.run(xcmp::DriverAction::EMIT_BINARY, program, false, path.c_str());
    // Initialise in/out buffers.
    simInBuffer.str(input);
    simInBuffer.clear();
    simOutBuffer.str("");
    simOutBuffer.clear();
    // Run the program.
    hexsim::Processor processor(simInBuffer, simOutBuffer);
    processor.load(path.c_str());
    processor.setTracing(trace);
    processor.setTruncateInputs(false);
    return processor.run();
  }

  /// Run an X program from a file.
  int runXProgramFile(const std::string filename,
                      const std::string input="",
                      bool trace=false) {
    return runXProgramSrc(readFile(filename), input, trace);
  }
};

#endif // TEST_CONTEXT_HPP
