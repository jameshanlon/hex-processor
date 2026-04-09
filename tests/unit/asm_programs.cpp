#include <catch2/catch_test_macros.hpp>
#include "TestContext.hpp"


//===---------------------------------------------------------------------===//
// Unit tests for X programs compiled with xhexb.S
//===---------------------------------------------------------------------===//

TEST_CASE("[asm_programs] xhexb_hello_putval") {
  TestContext ctx;
  // Assemble and run compilation.
  auto fileContents = ctx.readFile(ctx.getXTestPath("hello_putval.x"));
  ctx.runHexProgramFile(ctx.getAsmTestPath("xhexb.S"), fileContents);
  // Run the compiled program.
  ctx.simXBinary("simout2");
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}

TEST_CASE("[asm_programs] xhexb_hello_prints") {
  TestContext ctx;
  // Assemble and run compilation.
  auto fileContents = ctx.readFile(ctx.getXTestPath("hello_prints.x"));
  ctx.runHexProgramFile(ctx.getAsmTestPath("xhexb.S"), fileContents);
  // Run the compiled program.
  ctx.simXBinary("simout2");
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}

TEST_CASE("[asm_programs] xhexb_xhexb_hello_prints") {
  TestContext ctx;
  auto xhexbContents = ctx.readFile(ctx.getXTestPath("xhexb.x"));
  auto helloContents = ctx.readFile(ctx.getXTestPath("hello_prints.x"));
  // Assemble xhexb and run compilation.
  ctx.runHexProgramFile(ctx.getAsmTestPath("xhexb.S"), xhexbContents);
  // Simulate the compiled program (compile xhexb using hexasm:xhexb.S binary).
  ctx.simXBinary("simout2", xhexbContents);
  REQUIRE(ctx.simOutBuffer.str() == R"(error near line 3054: illegal character
tree size: 18631
program size: 17093
size: 177097
)");
  // Simulate the compiled program (compile hello_prints using xhexb.S:xhexb.x binary).
  ctx.simXBinary("simout2", helloContents);
  REQUIRE(ctx.simOutBuffer.str() == R"(error near line 74: illegal character
tree size: 602
program size: 414
size: 414
)");
  // Run the compiled program (hello_prints binary).
  ctx.simXBinary("simout2");
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}