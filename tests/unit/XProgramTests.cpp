#include "TestContext.hpp"
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

//===---------------------------------------------------------------------===//
// Unit tests for X programs.
//===---------------------------------------------------------------------===//

TEST_CASE("Mul", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul.x"), {1, 1}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul.x"), {3, 13}) == 39);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul.x"), {13, 3}) == 39);
}

TEST_CASE("Div", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("div.x"), {1, 1}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("div.x"), {13, 3}) == 4);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("div.x"), {3, 13}) == 0);
}

TEST_CASE("Fib", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {0}) == 0);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {1}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {2}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {3}) == 2);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {4}) == 3);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {5}) == 5);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fib.x"), {6}) == 8);
}

TEST_CASE("Fac", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fac.x"), {0}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fac.x"), {1}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fac.x"), {2}) == 2);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fac.x"), {3}) == 6);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fac.x"), {4}) == 24);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("fac.x"), {5}) == 120);
}

TEST_CASE("Mul2", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul2.x"), {1, 4}) == 4);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul2.x"), {2, 4}) == 8);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul2.x"), {3, 4}) == 12);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("mul2.x"), {4, 4}) == 16);
}

TEST_CASE("Exp2", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("exp2.x"), {1}) == 2);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("exp2.x"), {2}) == 4);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("exp2.x"), {3}) == 8);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("exp2.x"), {4}) == 16);
}

TEST_CASE("Hello putval", "[x_programs]") {
  TestContext ctx;

  ctx.runXProgramFile(ctx.getXTestPath("hello_putval.x"));
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}

TEST_CASE("Hello prints", "[x_programs]") {
  TestContext ctx;

  ctx.runXProgramFile(ctx.getXTestPath("hello_prints.x"));
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}

TEST_CASE("Printn", "[x_programs]") {
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

TEST_CASE("Printhex", "[x_programs]") {
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

TEST_CASE("Exit", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("exit.x")) == 0);
}

TEST_CASE("Echo char", "[x_programs]") {
  TestContext ctx;

  ctx.runXProgramFile(ctx.getXTestPath("echo_char.x"), "z");
  REQUIRE(ctx.simOutBuffer.str() == "z");
}

TEST_CASE("Strlen", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("strlen.x")) == 3);
}

TEST_CASE("Bubblesort", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("bubblesort.x")) == 0);
}

TEST_CASE("Gcd", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("gcd.x"), {48, 36}) == 12);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("gcd.x"), {17, 5}) == 1);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("gcd.x"), {100, 60}) == 20);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("gcd.x"), {7, 0}) == 7);
}

TEST_CASE("Collatz", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("collatz.x"), {1}) == 0);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("collatz.x"), {6}) == 8);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("collatz.x"), {7}) == 16);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("collatz.x"), {12}) == 9);
}

TEST_CASE("Ackermann", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("ackermann.x"), {1, 1}) == 3);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("ackermann.x"), {2, 2}) == 7);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("ackermann.x"), {2, 3}) == 9);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("ackermann.x"), {3, 3}) == 61);
}

TEST_CASE("Binsearch", "[x_programs]") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("binsearch.x"), {2}) == 0);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("binsearch.x"), {14}) == 6);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("binsearch.x"), {20}) == 9);
  REQUIRE(ctx.runXProgramFile(ctx.getXTestPath("binsearch.x"), {7}) == 99);
}

TEST_CASE("Reverse", "[x_programs]") {
  TestContext ctx;

  ctx.runXProgramFile(ctx.getXTestPath("reverse.x"), {5});
  REQUIRE(ctx.simOutBuffer.str() == "43210\n");
}

TEST_CASE("Primes", "[x_programs]") {
  TestContext ctx;

  ctx.runXProgramFile(ctx.getXTestPath("primes.x"), {20});
  REQUIRE(ctx.simOutBuffer.str() == "2\n3\n5\n7\n11\n13\n17\n19\n");
}

TEST_CASE("Hanoi", "[x_programs]") {
  TestContext ctx;

  ctx.runXProgramFile(ctx.getXTestPath("hanoi.x"), {3});
  REQUIRE(ctx.simOutBuffer.str() ==
          "A -> C\nA -> B\nC -> B\nA -> C\nB -> A\nB -> C\nA -> C\n");
}

TEST_CASE("Xhexb tree", "[x_programs]") {
  TestContext ctx;

  // Demonstrate the xhexb.x can be parsed into an AST.
  REQUIRE(!ctx.treeXProgramFile(ctx.getXTestPath("xhexb.x")).str().empty());
}

TEST_CASE("Xhexb hello putval", "[x_programs]") {
  TestContext ctx;

  // Compile xhexb, compile program.
  auto fileContents = ctx.readFile(ctx.getXTestPath("hello_putval.x"));
  ctx.runXProgramFile(ctx.getXTestPath("xhexb.x"), fileContents);
  // Simulate the compiled program.
  ctx.simXBinary("simout2");
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}

TEST_CASE("Xhexb hello prints", "[x_programs]") {
  TestContext ctx;

  // Compile xhexb, compile program.
  auto fileContents = ctx.readFile(ctx.getXTestPath("hello_prints.x"));
  ctx.runXProgramFile(ctx.getXTestPath("xhexb.x"), fileContents);
  // Simulate the compiled program.
  ctx.simXBinary("simout2");
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}

TEST_CASE("Xhexb xhexb hello prints", "[x_programs]") {
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
