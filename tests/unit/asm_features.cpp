#include <ostream>
#include <iostream>
#include <catch2/catch_test_macros.hpp>
#include "TestContext.hpp"


//===---------------------------------------------------------------------===//
// Unit tests for assembly languages features.
//===---------------------------------------------------------------------===//

TEST_CASE("[asm_features] exit_tokens") {
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

TEST_CASE("[asm_features] exit_tree") {
  TestContext ctx;
  auto output = ctx.asmHexProgramFile(ctx.getAsmTestPath("exit0.S"), true).str();
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

TEST_CASE("[asm_features] exit_bin") {
  TestContext ctx;
  auto output = ctx.asmHexProgramFile(ctx.getAsmTestPath("exit0.S"));
  output.seekp(0, std::ios::end);
  REQUIRE(output.tellp() == 16);
}

TEST_CASE("[asm_features] exit0_run") {
  TestContext ctx;
  auto ret = ctx.runHexProgramFile(ctx.getAsmTestPath("exit0.S"));
  REQUIRE(ret == 0);
}

TEST_CASE("[asm_features] exit255_run") {
  TestContext ctx;
  auto ret = ctx.runHexProgramFile(ctx.getAsmTestPath("exit255.S"));
  REQUIRE(ret == 255);
}

TEST_CASE("[asm_features] hello_run") {
  TestContext ctx;
  ctx.runHexProgramFile(ctx.getAsmTestPath("hello.S"));
  REQUIRE(ctx.simOutBuffer.str() == "hello\n");
}

TEST_CASE("[asm_features] hello_procedure_run") {
  TestContext ctx;
  ctx.runHexProgramFile(ctx.getAsmTestPath("hello_procedure.S"));
  REQUIRE(ctx.simOutBuffer.str() == "hello\n");
}

//===---------------------------------------------------------------------===//
// Error handling.
//===---------------------------------------------------------------------===//

TEST_CASE("[asm_features] error_unexpected_opr_operand") {
  TestContext ctx;
  auto program = "OPR OPR";
  REQUIRE_THROWS_AS(ctx.asmHexProgramSrc(program), hexasm::InvalidOprError);
}

TEST_CASE("[asm_features] error_unrecognised_token") {
  TestContext ctx;
  auto program = "123";
  REQUIRE_THROWS_AS(ctx.asmHexProgramSrc(program), hexasm::UnrecognisedTokenError);
}

TEST_CASE("[asm_features] error_expected_number") {
  TestContext ctx;
  auto program = "BR .";
  REQUIRE_THROWS_AS(ctx.asmHexProgramSrc(program), hexasm::UnexpectedTokenError);
}

TEST_CASE("[asm_features] error_expected_negative_integer") {
  TestContext ctx;
  auto program = "BR -foo";
  REQUIRE_THROWS_AS(ctx.asmHexProgramSrc(program), hexasm::UnexpectedTokenError);
}
