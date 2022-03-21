#include <ostream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include "TestContext.hpp"

/// Unit tests for assembly programs.

BOOST_FIXTURE_TEST_SUITE(asm_tests, TestContext);

BOOST_AUTO_TEST_CASE(exit_tokens) {
  auto program = ""
R"(LDAC 0
OPR SVC)";
  auto output = tokHexProgram(program).str();
  std::string expected = ""
R"(LDAC
NUMBER 0
OPR
SVC
EOF
)";
  BOOST_TEST(output == expected);
}

BOOST_AUTO_TEST_CASE(exit_tree) {
  auto program = ""
R"(LDAC 0
OPR SVC
)";
  auto output = asmHexProgram(program, false, true).str();
  std::string expected = ""
R"(00000000 LDAC 0               (1 bytes)
0x000001 OPR SVC              (1 bytes)
00000000 PADDING 2            (2 bytes)
2 bytes
)";
  BOOST_TEST(output == expected);
}

BOOST_AUTO_TEST_CASE(exit_bin) {
  auto program = ""
R"(LDAC 0
OPR SVC)";
  auto output = asmHexProgram(program);
  output.seekp(0, std::ios::end);
  BOOST_TEST(output.tellp() == 4);
}

BOOST_AUTO_TEST_CASE(exit_run) {
  auto program = ""
R"(BR start
DATA 16383
start
LDAC 0 # areg <- 0
LDBM 1 # breg <- sp
STAI 2 # sp[2] <- areg
LDAC 0
OPR SVC)";
  std::istringstream simInBuffer;
  std::ostringstream simOutBuffer;
  auto ret = runHexProgram(program, simInBuffer, simOutBuffer);
  BOOST_TEST(ret == 0);
}

BOOST_AUTO_TEST_CASE(exit0_run) {
  std::istringstream simInBuffer;
  std::ostringstream simOutBuffer;
  auto ret = runHexProgram(getAsmTestPath("exit0.S"), simInBuffer, simOutBuffer, true);
  BOOST_TEST(ret == 0);
}

BOOST_AUTO_TEST_CASE(exit255_run) {
  std::istringstream simInBuffer;
  std::ostringstream simOutBuffer;
  auto ret = runHexProgram(getAsmTestPath("exit255.S"), simInBuffer, simOutBuffer, true);
  BOOST_TEST(ret == 255);
}

BOOST_AUTO_TEST_CASE(hello_run) {
  std::istringstream simInBuffer;
  std::ostringstream simOutBuffer;
  runHexProgram(getAsmTestPath("hello.S"), simInBuffer, simOutBuffer, true);
  BOOST_TEST(simOutBuffer.str() == "hello\n");
}

BOOST_AUTO_TEST_CASE(hello_procedure_run) {
  std::istringstream simInBuffer;
  std::ostringstream simOutBuffer;
  runHexProgram(getAsmTestPath("hello_procedure.S"), simInBuffer, simOutBuffer, true);
  BOOST_TEST(simOutBuffer.str() == "hello\n");
}

BOOST_AUTO_TEST_SUITE_END();
