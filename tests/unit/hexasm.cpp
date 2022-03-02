#include <ostream>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include "TestContext.hpp"

BOOST_FIXTURE_TEST_SUITE(hexasm, TestContext);

BOOST_AUTO_TEST_CASE(exit_tokens) {
  auto program = ""
R"(LDAC 0
OPR SVC)";
  auto output = tokeniseAsmProgram(program);
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
  auto output = treeAsmProgram(program);
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
  auto output = assembleAsmProgram(program);
  output.seekp(0, std::ios::end);
  BOOST_TEST(output.tellp() == 4);
}

BOOST_AUTO_TEST_SUITE_END();
