#include "TestContext.hpp"
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

//===---------------------------------------------------------------------===//
// Unit tests for X programs.
//===---------------------------------------------------------------------===//

TEST_CASE("mul") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul.x"), {1, 1}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul.x"), {3, 13}) == 39);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul.x"), {13, 3}) == 39);
}

TEST_CASE("div") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("div.x"), {1, 1}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("div.x"), {13, 3}) == 4);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("div.x"), {3, 13}) == 0);
}

TEST_CASE("fib") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {0}) == 0);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {1}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {2}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {3}) == 2);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {4}) == 3);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {5}) == 5);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {6}) == 8);
}

TEST_CASE("fac") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fac.x"), {0}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fac.x"), {1}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fac.x"), {2}) == 2);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fac.x"), {3}) == 6);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fac.x"), {4}) == 24);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fac.x"), {5}) == 120);
}

TEST_CASE("mul2") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul2.x"), {1, 4}) == 4);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul2.x"), {2, 4}) == 8);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul2.x"), {3, 4}) == 12);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul2.x"), {4, 4}) == 16);
}

TEST_CASE("exp2") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("exp2.x"), {1}) == 2);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("exp2.x"), {2}) == 4);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("exp2.x"), {3}) == 8);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("exp2.x"), {4}) == 16);
}

TEST_CASE("hello_putval") {
  TestContext ctx;

  ctx.runXProgramFile(ctx.getXTestPath("hello_putval.x"));
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}

TEST_CASE("hello_prints") {
  TestContext ctx;

  ctx.runXProgramFile(ctx.getXTestPath("hello_prints.x"));
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}

TEST_CASE("printn") {
  TestContext ctx;

  ctx.runXProgramFile(ctx.getXTestPath("printn.x"), {0});
  REQUIRE(ctx.simOutBuffer.str() == "0");
  ctx.runXProgramFile(ctx.getXTestPath("printn.x"), {1});
  REQUIRE(ctx.simOutBuffer.str() == "1");
  ctx.runXProgramFile(ctx.getXTestPath("printn.x"), {42});
  REQUIRE(ctx.simOutBuffer.str() == "42");
  ctx.runXProgramFile(ctx.getXTestPath("printn.x"), {127});
  REQUIRE(ctx.simOutBuffer.str() == "127");
}

TEST_CASE("printhex") {
  TestContext ctx;

  ctx.runXProgramFile(ctx.getXTestPath("printhex.x"), {0});
  REQUIRE(ctx.simOutBuffer.str() == "0");
  ctx.runXProgramFile(ctx.getXTestPath("printhex.x"), {1});
  REQUIRE(ctx.simOutBuffer.str() == "1");
  ctx.runXProgramFile(ctx.getXTestPath("printhex.x"), {42});
  REQUIRE(ctx.simOutBuffer.str() == "2a");
  ctx.runXProgramFile(ctx.getXTestPath("printhex.x"), {127});
  REQUIRE(ctx.simOutBuffer.str() == "7f");
}

TEST_CASE("strlen") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("strlen.x")) == 3);
}

TEST_CASE("bubblesort") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("bubblesort.x")) == 0);
}

TEST_CASE("xhexb_tree") {
  TestContext ctx;

  // Demonstrate the xhexb.x can be parsed into an AST.
  REQUIRE(!ctx.treeXProgramFile(ctx.getXTestPath("xhexb.x")).str().empty());
}

TEST_CASE("xhexb_hello_putval") {
  TestContext ctx;

  // Compile xhexb, compile program.
  auto fileContents = ctx.readFile(ctx.getXTestPath("hello_putval.x"));
  ctx.runXProgramFile(ctx.getXTestPath("xhexb.x"), fileContents);
  // Simulate the compiled program.
  ctx.simXBinary("simout2");
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}

TEST_CASE("xhexb_hello_prints") {
  TestContext ctx;

  // Compile xhexb, compile program.
  auto fileContents = ctx.readFile(ctx.getXTestPath("hello_prints.x"));
  ctx.runXProgramFile(ctx.getXTestPath("xhexb.x"), fileContents);
  // Simulate the compiled program.
  ctx.simXBinary("simout2");
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}

TEST_CASE("xhexb_xhexb_hello_prints") {
  TestContext ctx;

  auto xhexbContents = ctx.readFile(ctx.getXTestPath("xhexb.x"));
  auto helloContents = ctx.readFile(ctx.getXTestPath("hello_prints.x"));
  // Compile xhexb, compile program.
  ctx.runXProgramFile(ctx.getXTestPath("xhexb.x"), xhexbContents);
  // Simulate the compiled program (compile xhexb using xcmp.cpp:xhexb.x
  // binary).
  ctx.simXBinary("simout2", xhexbContents);
  REQUIRE(ctx.simOutBuffer.str() == R"(error near line 3054: illegal character
tree size: 18631
program size: 17093
size: 177097
)");
  // Simulate the compiled program (compile hello_prints using xhexb.x:xhexb.x
  // binary).
  ctx.simXBinary("simout2", helloContents);
  REQUIRE(ctx.simOutBuffer.str() == R"(error near line 74: illegal character
tree size: 602
program size: 414
size: 414
)");
  // Simulate the compiled program (hello_prints binary).
  ctx.simXBinary("simout2");
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}
