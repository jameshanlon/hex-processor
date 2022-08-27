#include <ostream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include "TestContext.hpp"


BOOST_FIXTURE_TEST_SUITE(x_programs, TestContext)

//===---------------------------------------------------------------------===//
// Unit tests for X programs.
//===---------------------------------------------------------------------===//

BOOST_AUTO_TEST_CASE(hello_putval) {
  runXProgramFile(getXTestPath("hello.x"), {1, 1});
  BOOST_TEST(simOutBuffer.str() == "hello\n");
}

//BOOST_AUTO_TEST_CASE(hello_prints) {
//  runXProgramFile(getXTestPath("prints.x"), {1, 1});
//  BOOST_TEST(simOutBuffer.str() == "hello\n");
//}

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

BOOST_AUTO_TEST_SUITE_END()
