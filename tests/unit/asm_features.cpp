#include <ostream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include "TestContext.hpp"


//===---------------------------------------------------------------------===//
// Unit tests for assembly languages features.
//===---------------------------------------------------------------------===//

BOOST_FIXTURE_TEST_SUITE(asm_features, TestContext)

BOOST_AUTO_TEST_CASE(exit_tokens) {
  auto output = tokHexProgramFile(getAsmTestPath("exit0.S")).str();
  std::string expected = ""
R"(BR
IDENTIFIER start
DATA
NUMBER 16383
IDENTIFIER start
LDAC
NUMBER 0
LDBM
NUMBER 1
STAI
NUMBER 2
LDAC
NUMBER 0
OPR
SVC
EOF
)";
  BOOST_TEST(output == expected);
}

BOOST_AUTO_TEST_CASE(exit_tree) {
  auto output = asmHexProgramFile(getAsmTestPath("exit0.S"), true).str();
  std::string expected = ""
R"(00000000 BR start (7)         (1 bytes)
0x000004 DATA 16383           (4 bytes)
0x000008 start                (0 bytes)
0x000008 LDAC 0               (1 bytes)
0x000009 LDBM 1               (1 bytes)
0x00000a STAI 2               (1 bytes)
0x00000b LDAC 0               (1 bytes)
0x00000c OPR SVC              (1 bytes)
00000000 PADDING 3            (3 bytes)
3 bytes
)";
  BOOST_TEST(output == expected);
}

BOOST_AUTO_TEST_CASE(exit_bin) {
  auto output = asmHexProgramFile(getAsmTestPath("exit0.S"));
  output.seekp(0, std::ios::end);
  BOOST_TEST(output.tellp() == 16);
}

BOOST_AUTO_TEST_CASE(exit0_run) {
  auto ret = runHexProgramFile(getAsmTestPath("exit0.S"));
  BOOST_TEST(ret == 0);
}

BOOST_AUTO_TEST_CASE(exit255_run) {
  auto ret = runHexProgramFile(getAsmTestPath("exit255.S"));
  BOOST_TEST(ret == 255);
}

BOOST_AUTO_TEST_CASE(hello_run) {
  runHexProgramFile(getAsmTestPath("hello.S"));
  BOOST_TEST(simOutBuffer.str() == "hello\n");
}

BOOST_AUTO_TEST_CASE(hello_procedure_run) {
  runHexProgramFile(getAsmTestPath("hello_procedure.S"));
  BOOST_TEST(simOutBuffer.str() == "hello\n");
}

//===---------------------------------------------------------------------===//
// Error handling.
//===---------------------------------------------------------------------===//

BOOST_AUTO_TEST_CASE(error_unexpected_opr_operand) {
  auto program = "OPR OPR";
  BOOST_CHECK_THROW(asmHexProgramSrc(program), hexasm::InvalidOprError);
}

BOOST_AUTO_TEST_CASE(error_unrecognised_token) {
  auto program = "123";
  BOOST_CHECK_THROW(asmHexProgramSrc(program), hexasm::UnrecognisedTokenError);
}

BOOST_AUTO_TEST_CASE(error_expected_number) {
  auto program = "BR .";
  BOOST_CHECK_THROW(asmHexProgramSrc(program), hexasm::UnexpectedTokenError);
}

BOOST_AUTO_TEST_CASE(error_expected_negative_integer) {
  auto program = "BR -foo";
  BOOST_CHECK_THROW(asmHexProgramSrc(program), hexasm::UnexpectedTokenError);
}

BOOST_AUTO_TEST_CASE(error_unknown_label) {
  auto program = "BR foo";
  BOOST_CHECK_THROW(asmHexProgramSrc(program), hexasm::UnknownLabelError);
}

BOOST_AUTO_TEST_SUITE_END()
