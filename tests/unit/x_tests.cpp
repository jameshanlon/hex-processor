#include <ostream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include "TestContext.hpp"

//===---------------------------------------------------------------------===//
// Unit tests for X programs.
//===---------------------------------------------------------------------===//

BOOST_FIXTURE_TEST_SUITE(x_tests, TestContext);

BOOST_AUTO_TEST_CASE(empty_run) {
  auto program = R"(
val foo = 1;
)";
  runXProgram(program);
}

BOOST_AUTO_TEST_CASE(hello_run) {
  auto program = R"(
val put = 1;
proc main() is {
  putval('h');
  putval('e');
  putval('l');
  putval('l');
  putval('o');
  newline()
}
proc putval(val c) is put(c, 0)
proc newline() is putval('\n')
)";
  runXProgram(program);
}

BOOST_AUTO_TEST_CASE(xhexb_run) {
  treeXProgram(getXTestPath("xhexb.x"), true);
}

BOOST_AUTO_TEST_SUITE_END();
