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
//  runXProgram(program);
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
//  runXProgram(program);
}

BOOST_AUTO_TEST_CASE(xhexb_run) {
  treeXProgram(getXTestPath("xhexb.x"), true);
}

//===---------------------------------------------------------------------===//
// Error handling.
//===---------------------------------------------------------------------===//

// Token errors

BOOST_AUTO_TEST_CASE(token_error_char_const_escape) {
  auto program = "val foo = '\\x';";
  BOOST_CHECK_THROW(asmXProgram(program), xcmp::CharConstError);
}

BOOST_AUTO_TEST_CASE(token_error_eq) {
  auto program = "val foo :~";
  BOOST_CHECK_THROW(asmXProgram(program), xcmp::TokenError);
}

BOOST_AUTO_TEST_CASE(token_error_char_const) {
  auto program = "val foo = 'x~";
  BOOST_CHECK_THROW(asmXProgram(program), xcmp::TokenError);
}

BOOST_AUTO_TEST_CASE(token_error_string) {
  auto program = "val foo = \"x";
  BOOST_CHECK_THROW(asmXProgram(program), xcmp::TokenError);
}

BOOST_AUTO_TEST_CASE(token_error_unexpected_char) {
  auto program = "val foo = ?";
  BOOST_CHECK_THROW(asmXProgram(program), xcmp::TokenError);
}

// Unexpected token errors

BOOST_AUTO_TEST_CASE(unexpected_token_error_elem) {
  auto program1 = "val foo = bar[100~";
  BOOST_CHECK_THROW(asmXProgram(program1), xcmp::UnexpectedTokenError);
  auto program2 = "proc foo() is bar(0~";
  BOOST_CHECK_THROW(asmXProgram(program2), xcmp::UnexpectedTokenError);
  auto program3 = "val foo = (0~";
  BOOST_CHECK_THROW(asmXProgram(program3), xcmp::UnexpectedTokenError);
}

BOOST_AUTO_TEST_CASE(unexpected_token_error_val_decl) {
  auto program1 = "val foo ~";
  BOOST_CHECK_THROW(asmXProgram(program1), xcmp::UnexpectedTokenError);
  auto program2 = "val foo = 1";
  BOOST_CHECK_THROW(asmXProgram(program2), xcmp::UnexpectedTokenError);
}

BOOST_AUTO_TEST_CASE(unexpected_token_error_array_decl) {
  auto program1 = "array foo~";
  BOOST_CHECK_THROW(asmXProgram(program1), xcmp::UnexpectedTokenError);
  auto program2 = "array foo[100~";
  BOOST_CHECK_THROW(asmXProgram(program2), xcmp::UnexpectedTokenError);
  auto program3 = "array foo[100]~";
  BOOST_CHECK_THROW(asmXProgram(program3), xcmp::UnexpectedTokenError);
}

BOOST_AUTO_TEST_CASE(unexpected_token_error_stmt_if) {
  auto program1 = "proc foo() is if 0 xxx";
  BOOST_CHECK_THROW(asmXProgram(program1), xcmp::UnexpectedTokenError);
  auto program2 = "proc foo() is if 0 then skip xxx";
  BOOST_CHECK_THROW(asmXProgram(program2), xcmp::UnexpectedTokenError);
}

BOOST_AUTO_TEST_CASE(unexpected_token_error_stmt_while) {
  auto program = "proc foo() is while 0 xxx";
  BOOST_CHECK_THROW(asmXProgram(program), xcmp::UnexpectedTokenError);
}

BOOST_AUTO_TEST_CASE(unexpected_token_error_stmt_block) {
  auto program = "proc foo() is { skip ~";
  BOOST_CHECK_THROW(asmXProgram(program), xcmp::UnexpectedTokenError);
}

BOOST_AUTO_TEST_CASE(unexpected_token_error_stmt_ass) {
  auto program = "proc foo() is bar ~";
  BOOST_CHECK_THROW(asmXProgram(program), xcmp::UnexpectedTokenError);
}

BOOST_AUTO_TEST_CASE(unexpected_token_error_proc_decl) {
  auto program0 = "proc foo~";
  BOOST_CHECK_THROW(asmXProgram(program0), xcmp::UnexpectedTokenError);
  auto program1 = "proc foo(val a ~";
  BOOST_CHECK_THROW(asmXProgram(program1), xcmp::UnexpectedTokenError);
  auto program2 = "proc foo() ~";
  BOOST_CHECK_THROW(asmXProgram(program2), xcmp::UnexpectedTokenError);
  auto program3 = "proc foo() is skip x x";
  BOOST_CHECK_THROW(asmXProgram(program3), xcmp::UnexpectedTokenError);
}

// Expected name error.

BOOST_AUTO_TEST_CASE(expected_name_error) {
  auto program0 = "proc ~";
  BOOST_CHECK_THROW(asmXProgram(program0), xcmp::ExpectedNameError);
}

// Parser token errors.

BOOST_AUTO_TEST_CASE(parser_token_error_elem) {
  auto program = "val foo = +";
  BOOST_CHECK_THROW(asmXProgram(program), xcmp::ParserTokenError);
}

BOOST_AUTO_TEST_CASE(parser_token_error_decl) {
  auto program = "proc foo() is xxx :=";
  BOOST_CHECK_THROW(asmXProgram(program), xcmp::ParserTokenError);
}

BOOST_AUTO_TEST_CASE(parser_token_error_formal) {
  auto program = "proc foo(val a, foo b) is skip";
  BOOST_CHECK_THROW(asmXProgram(program), xcmp::ParserTokenError);
}

BOOST_AUTO_TEST_CASE(parser_token_error_stmt_invalid) {
  auto program = "proc foo() is ~";
  BOOST_CHECK_THROW(asmXProgram(program), xcmp::ParserTokenError);
}

BOOST_AUTO_TEST_SUITE_END();
