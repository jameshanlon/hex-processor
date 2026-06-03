#include "TestContext.hpp"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <ostream>

//===---------------------------------------------------------------------===//
// Unit tests for assembly languages features.
//===---------------------------------------------------------------------===//

TEST_CASE("Exit tokens", "[asm_features]") {
  TestContext ctx;
  auto output = ctx.tokHexProgramFile(ctx.getAsmTestPath("exit0.S")).str();
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
  REQUIRE(output == expected);
}

TEST_CASE("Exit tree", "[asm_features]") {
  TestContext ctx;
  auto output =
      ctx.asmHexProgramFile(ctx.getAsmTestPath("exit0.S"), true).str();
  std::string expected = ""
                         R"(0x00000000 BR start (7)         (1 bytes)
0x00000004 DATA 16383           (4 bytes)
0x00000008 start                (0 bytes)
0x00000008 LDAC 0               (1 bytes)
0x00000009 LDBM 1               (1 bytes)
0x0000000a STAI 2               (1 bytes)
0x0000000b LDAC 0               (1 bytes)
0x0000000c OPR SVC              (1 bytes)
0x00000000 PADDING 3            (3 bytes)
13 bytes
)";
  REQUIRE(output == expected);
}

TEST_CASE("Exit bin", "[asm_features]") {
  TestContext ctx;
  auto output = ctx.asmHexProgramFile(ctx.getAsmTestPath("exit0.S"));
  output.seekp(0, std::ios::end);
  REQUIRE(output.tellp() == 16);
}

TEST_CASE("Exit0 run", "[asm_features]") {
  TestContext ctx;
  auto ret = ctx.runHexProgramFile(ctx.getAsmTestPath("exit0.S"));
  REQUIRE(ret == 0);
}

TEST_CASE("Exit255 run", "[asm_features]") {
  TestContext ctx;
  auto ret = ctx.runHexProgramFile(ctx.getAsmTestPath("exit255.S"));
  REQUIRE(ret == 255);
}

TEST_CASE("Hello run", "[asm_features]") {
  TestContext ctx;
  ctx.runHexProgramFile(ctx.getAsmTestPath("hello.S"));
  REQUIRE(ctx.simOutBuffer.str() == "hello\n");
}

TEST_CASE("Hello procedure run", "[asm_features]") {
  TestContext ctx;
  ctx.runHexProgramFile(ctx.getAsmTestPath("hello_procedure.S"));
  REQUIRE(ctx.simOutBuffer.str() == "hello\n");
}

//===---------------------------------------------------------------------===//
// Error handling.
//===---------------------------------------------------------------------===//

TEST_CASE("Error unexpected opr operand", "[asm_features]") {
  TestContext ctx;
  auto program = "OPR OPR";
  REQUIRE_THROWS_AS(ctx.asmHexProgramSrc(program), hexasm::InvalidOprError);
}

TEST_CASE("Error unrecognised token", "[asm_features]") {
  TestContext ctx;
  auto program = "123";
  REQUIRE_THROWS_AS(ctx.asmHexProgramSrc(program),
                    hexasm::UnrecognisedTokenError);
}

TEST_CASE("Error expected number", "[asm_features]") {
  TestContext ctx;
  auto program = "BR .";
  REQUIRE_THROWS_AS(ctx.asmHexProgramSrc(program),
                    hexasm::UnexpectedTokenError);
}

TEST_CASE("Error expected negative integer", "[asm_features]") {
  TestContext ctx;
  auto program = "BR -foo";
  REQUIRE_THROWS_AS(ctx.asmHexProgramSrc(program),
                    hexasm::UnexpectedTokenError);
}
