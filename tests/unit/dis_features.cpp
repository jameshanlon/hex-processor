#include "TestContext.hpp"
#include "hexdis.hpp"
#include <catch2/catch_test_macros.hpp>

//===---------------------------------------------------------------------===//
// Unit tests for disassembler features.
//===---------------------------------------------------------------------===//

/// Helper: assemble source, write to temp binary, disassemble it.
static std::string assembleAndDisassemble(TestContext &ctx,
                                          const std::string &asmSrc,
                                          bool showLabels = false) {
  // Assemble to a binary file.
  hexasm::Lexer lexer;
  hexasm::Parser parser(lexer);
  lexer.loadBuffer(asmSrc);
  auto tree = parser.parseProgram();
  hexasm::CodeGen codeGen(tree);
  fs::path path(CURRENT_BINARY_DIRECTORY);
  path /= "dis_test.bin";
  codeGen.emitBin(path.c_str());
  // Read binary back and disassemble.
  std::vector<uint8_t> program;
  hexdis::DebugInfo debugInfo;
  hexdis::readBinaryFile(path.c_str(), program, debugInfo);
  std::ostringstream out;
  hexdis::disassemble(program, out, debugInfo, showLabels);
  return out.str();
}

TEST_CASE("[dis_features] simple_instructions") {
  TestContext ctx;
  // A minimal program: LDAC 5, LDBC 3, OPR ADD.
  auto output = assembleAndDisassemble(ctx, "LDAC 5\n"
                                            "LDBC 3\n"
                                            "OPR ADD\n");
  REQUIRE(output.find("LDAC 5") != std::string::npos);
  REQUIRE(output.find("LDBC 3") != std::string::npos);
  REQUIRE(output.find("ADD") != std::string::npos);
}

TEST_CASE("[dis_features] all_basic_opcodes") {
  TestContext ctx;
  auto output = assembleAndDisassemble(ctx, "LDAM 1\n"
                                            "LDBM 2\n"
                                            "STAM 3\n"
                                            "LDAC 4\n"
                                            "LDBC 5\n"
                                            "LDAP 6\n"
                                            "LDAI 7\n"
                                            "LDBI 8\n"
                                            "STAI 9\n"
                                            "BR 0\n"
                                            "BRZ 0\n"
                                            "BRN 0\n");
  REQUIRE(output.find("LDAM 1") != std::string::npos);
  REQUIRE(output.find("LDBM 2") != std::string::npos);
  REQUIRE(output.find("STAM 3") != std::string::npos);
  REQUIRE(output.find("LDAC 4") != std::string::npos);
  REQUIRE(output.find("LDBC 5") != std::string::npos);
  REQUIRE(output.find("LDAP 6") != std::string::npos);
  REQUIRE(output.find("LDAI 7") != std::string::npos);
  REQUIRE(output.find("LDBI 8") != std::string::npos);
  REQUIRE(output.find("STAI 9") != std::string::npos);
  REQUIRE(output.find("BR   0") != std::string::npos);
  REQUIRE(output.find("BRZ  0") != std::string::npos);
  REQUIRE(output.find("BRN  0") != std::string::npos);
}

TEST_CASE("[dis_features] opr_sub_instructions") {
  TestContext ctx;
  auto output = assembleAndDisassemble(ctx, "OPR BRB\n"
                                            "OPR ADD\n"
                                            "OPR SUB\n"
                                            "OPR SVC\n");
  REQUIRE(output.find("BRB") != std::string::npos);
  REQUIRE(output.find("ADD") != std::string::npos);
  REQUIRE(output.find("SUB") != std::string::npos);
  REQUIRE(output.find("SVC") != std::string::npos);
}

TEST_CASE("[dis_features] pfix_extended_operand") {
  TestContext ctx;
  // LDAC 32 requires PFIX 2, LDAC 0 -> operand becomes 0x20 = 32.
  auto output = assembleAndDisassemble(ctx, "LDAC 32\n");
  REQUIRE(output.find("PFIX 2") != std::string::npos);
  REQUIRE(output.find("LDAC 32") != std::string::npos);
}

TEST_CASE("[dis_features] nfix_negative_operand") {
  TestContext ctx;
  // LDAC -1 requires NFIX then LDAC with accumulated negative operand.
  auto output = assembleAndDisassemble(ctx, "LDAC -1\n");
  REQUIRE(output.find("NFIX") != std::string::npos);
  REQUIRE(output.find("LDAC 4294967295") != std::string::npos);
}

TEST_CASE("[dis_features] multi_pfix_large_operand") {
  TestContext ctx;
  // LDAC 256 requires PFIX 1, PFIX 0, LDAC 0 -> 0x100 = 256.
  auto output = assembleAndDisassemble(ctx, "LDAC 256\n");
  REQUIRE(output.find("PFIX") != std::string::npos);
  REQUIRE(output.find("LDAC 256") != std::string::npos);
}

TEST_CASE("[dis_features] byte_hex_values") {
  TestContext ctx;
  // Check that raw hex byte values are shown.
  auto output = assembleAndDisassemble(ctx, "LDAC 5\n");
  // LDAC opcode is 0x3, operand 5, so byte is 0x35.
  REQUIRE(output.find("35") != std::string::npos);
}

TEST_CASE("[dis_features] address_shown") {
  TestContext ctx;
  auto output = assembleAndDisassemble(ctx, "LDAC 1\n"
                                            "LDAC 2\n");
  // First instruction at 0x0000, second at 0x0001.
  REQUIRE(output.find("0x0000") != std::string::npos);
  REQUIRE(output.find("0x0001") != std::string::npos);
}

TEST_CASE("[dis_features] exit0_roundtrip") {
  TestContext ctx;
  auto output =
      assembleAndDisassemble(ctx, ctx.readFile(ctx.getAsmTestPath("exit0.S")));
  // The program starts with a branch and contains SVC for exit.
  REQUIRE(output.find("BR") != std::string::npos);
  REQUIRE(output.find("LDAC 0") != std::string::npos);
  REQUIRE(output.find("SVC") != std::string::npos);
}

TEST_CASE("[dis_features] hello_roundtrip") {
  TestContext ctx;
  auto output =
      assembleAndDisassemble(ctx, ctx.readFile(ctx.getAsmTestPath("hello.S")));
  REQUIRE(output.find("BR") != std::string::npos);
  REQUIRE(output.find("STAI") != std::string::npos);
  REQUIRE(output.find("SVC") != std::string::npos);
}

TEST_CASE("[dis_features] x_program_labels") {
  TestContext ctx;
  // Compile an X program (which generates debug info with labels).
  std::ostringstream driverOut;
  xcmp::Driver driver(driverOut);
  fs::path path(CURRENT_BINARY_DIRECTORY);
  path /= "dis_label_test.bin";
  std::string xSrc = ctx.readFile(ctx.getXTestPath("hello_putval.x"));
  driver.run(xcmp::DriverAction::EMIT_BINARY, xSrc, false, path.c_str());
  // Read and disassemble with labels.
  std::vector<uint8_t> program;
  hexdis::DebugInfo debugInfo;
  hexdis::readBinaryFile(path.c_str(), program, debugInfo);
  std::ostringstream out;
  hexdis::disassemble(program, out, debugInfo, true);
  auto output = out.str();
  REQUIRE(output.find("main:") != std::string::npos);
  REQUIRE(output.find("putval:") != std::string::npos);
}

TEST_CASE("[dis_features] x_program_no_labels") {
  TestContext ctx;
  // Compile an X program and disassemble without labels.
  std::ostringstream driverOut;
  xcmp::Driver driver(driverOut);
  fs::path path(CURRENT_BINARY_DIRECTORY);
  path /= "dis_nolabel_test.bin";
  std::string xSrc = ctx.readFile(ctx.getXTestPath("hello_putval.x"));
  driver.run(xcmp::DriverAction::EMIT_BINARY, xSrc, false, path.c_str());
  // Read and disassemble without labels.
  std::vector<uint8_t> program;
  hexdis::DebugInfo debugInfo;
  hexdis::readBinaryFile(path.c_str(), program, debugInfo);
  std::ostringstream out;
  hexdis::disassemble(program, out, debugInfo, false);
  auto output = out.str();
  REQUIRE(output.find("main:") == std::string::npos);
  REQUIRE(output.find("putval:") == std::string::npos);
  // But instructions should still be present.
  REQUIRE(output.find("LDAC") != std::string::npos);
}
