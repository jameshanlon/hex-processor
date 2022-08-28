#include <ostream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include "TestContext.hpp"


BOOST_FIXTURE_TEST_SUITE(x_programs, TestContext)

//===---------------------------------------------------------------------===//
// Unit tests for X programs.
//===---------------------------------------------------------------------===//

BOOST_AUTO_TEST_CASE(mul) {
  BOOST_TEST(runXProgramFile(getXTestPath("mul.x"), {1, 1}) == 1);
  BOOST_TEST(runXProgramFile(getXTestPath("mul.x"), {3, 13}) == 39);
  BOOST_TEST(runXProgramFile(getXTestPath("mul.x"), {13, 3}) == 39);
}

BOOST_AUTO_TEST_CASE(div) {
  BOOST_TEST(runXProgramFile(getXTestPath("div.x"), {1, 1}) == 1);
  BOOST_TEST(runXProgramFile(getXTestPath("div.x"), {13, 3}) == 4);
  BOOST_TEST(runXProgramFile(getXTestPath("div.x"), {3, 13}) == 0);
}

BOOST_AUTO_TEST_CASE(fib) {
  BOOST_TEST(runXProgramFile(getXTestPath("fib.x"), {0}) == 0);
  BOOST_TEST(runXProgramFile(getXTestPath("fib.x"), {1}) == 1);
  BOOST_TEST(runXProgramFile(getXTestPath("fib.x"), {2}) == 1);
  BOOST_TEST(runXProgramFile(getXTestPath("fib.x"), {3}) == 2);
  BOOST_TEST(runXProgramFile(getXTestPath("fib.x"), {4}) == 3);
  BOOST_TEST(runXProgramFile(getXTestPath("fib.x"), {5}) == 5);
  BOOST_TEST(runXProgramFile(getXTestPath("fib.x"), {6}) == 8);
}

BOOST_AUTO_TEST_CASE(fac) {
  BOOST_TEST(runXProgramFile(getXTestPath("fac.x"), {0}) == 1);
  BOOST_TEST(runXProgramFile(getXTestPath("fac.x"), {1}) == 1);
  BOOST_TEST(runXProgramFile(getXTestPath("fac.x"), {2}) == 2);
  BOOST_TEST(runXProgramFile(getXTestPath("fac.x"), {3}) == 6);
  BOOST_TEST(runXProgramFile(getXTestPath("fac.x"), {4}) == 24);
  BOOST_TEST(runXProgramFile(getXTestPath("fac.x"), {5}) == 120);
}

BOOST_AUTO_TEST_CASE(mul2) {
  BOOST_TEST(runXProgramFile(getXTestPath("mul2.x"), {1, 4}) == 4);
  BOOST_TEST(runXProgramFile(getXTestPath("mul2.x"), {2, 4}) == 8);
  BOOST_TEST(runXProgramFile(getXTestPath("mul2.x"), {3, 4}) == 12);
  BOOST_TEST(runXProgramFile(getXTestPath("mul2.x"), {4, 4}) == 16);
}

BOOST_AUTO_TEST_CASE(exp2) {
  BOOST_TEST(runXProgramFile(getXTestPath("exp2.x"), {1}) == 2);
  BOOST_TEST(runXProgramFile(getXTestPath("exp2.x"), {2}) == 4);
  BOOST_TEST(runXProgramFile(getXTestPath("exp2.x"), {3}) == 8);
  BOOST_TEST(runXProgramFile(getXTestPath("exp2.x"), {4}) == 16);
}

BOOST_AUTO_TEST_CASE(hello_putval) {
  runXProgramFile(getXTestPath("hello_putval.x"));
  BOOST_TEST(simOutBuffer.str() == "hello world\n");
}

BOOST_AUTO_TEST_CASE(hello_prints) {
  runXProgramFile(getXTestPath("hello_prints.x"));
  BOOST_TEST(simOutBuffer.str() == "hello world\n");
}

BOOST_AUTO_TEST_CASE(printn) {
  runXProgramFile(getXTestPath("printn.x"), {0});
  BOOST_TEST(simOutBuffer.str() == "0");
  runXProgramFile(getXTestPath("printn.x"), {1});
  BOOST_TEST(simOutBuffer.str() == "1");
  runXProgramFile(getXTestPath("printn.x"), {42});
  BOOST_TEST(simOutBuffer.str() == "42");
  runXProgramFile(getXTestPath("printn.x"), {127});
  BOOST_TEST(simOutBuffer.str() == "127");
}

BOOST_AUTO_TEST_CASE(printhex) {
  runXProgramFile(getXTestPath("printhex.x"), {0});
  BOOST_TEST(simOutBuffer.str() == "0");
  runXProgramFile(getXTestPath("printhex.x"), {1});
  BOOST_TEST(simOutBuffer.str() == "1");
  runXProgramFile(getXTestPath("printhex.x"), {42});
  BOOST_TEST(simOutBuffer.str() == "2a");
  runXProgramFile(getXTestPath("printhex.x"), {127});
  BOOST_TEST(simOutBuffer.str() == "7f");
}

BOOST_AUTO_TEST_CASE(strlen) {
  BOOST_TEST(runXProgramFile(getXTestPath("strlen.x")) == 3);
}

BOOST_AUTO_TEST_SUITE_END()
