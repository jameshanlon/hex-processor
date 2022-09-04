#include <ostream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include "TestContext.hpp"


//===---------------------------------------------------------------------===//
// Unit tests for X programs compiled with xhexb.S
//===---------------------------------------------------------------------===//

BOOST_FIXTURE_TEST_SUITE(asm_programs, TestContext)

BOOST_AUTO_TEST_CASE(xhexb_hello_putval) {
  // Assemble and run compilation.
  auto fileContents = readFile(getXTestPath("hello_putval.x"));
  runHexProgramFile(getAsmTestPath("xhexb.S"), fileContents);
  // Run the compiled program.
  simXBinary("simout2");
  BOOST_TEST(simOutBuffer.str() == "hello world\n");
}

BOOST_AUTO_TEST_CASE(xhexb_hello_prints) {
  // Assemble and run compilation.
  auto fileContents = readFile(getXTestPath("hello_prints.x"));
  runHexProgramFile(getAsmTestPath("xhexb.S"), fileContents);
  // Run the compiled program.
  simXBinary("simout2");
  BOOST_TEST(simOutBuffer.str() == "hello world\n");
}

BOOST_AUTO_TEST_CASE(xhexb_xhexb_hello_prints) {
  auto xhexbContents = readFile(getXTestPath("xhexb.x"));
  auto helloContents = readFile(getXTestPath("hello_prints.x"));
  // Assemble xhexb and run compilation.
  runHexProgramFile(getAsmTestPath("xhexb.S"), xhexbContents);
  // Simulate the compiled program (compile xhexb using hexasm:xhexb.S binary).
  simXBinary("simout2", xhexbContents);
  BOOST_TEST(simOutBuffer.str() == R"(error near line 3054: illegal character
tree size: 18631
program size: 17093
size: 177097
)");
  // Simulate the compiled program (compile hello_prints using xhexb.S:xhexb.x binary).
  simXBinary("simout2", helloContents);
  BOOST_TEST(simOutBuffer.str() == R"(error near line 74: illegal character
tree size: 602
program size: 414
size: 414
)");
  // Run the compiled program (hello_prints binary).
  simXBinary("simout2");
  BOOST_TEST(simOutBuffer.str() == "hello world\n");
}

BOOST_AUTO_TEST_SUITE_END()
