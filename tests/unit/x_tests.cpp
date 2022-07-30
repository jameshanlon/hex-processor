#include <ostream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include "TestContext.hpp"

// Unit tests for X programs.

BOOST_FIXTURE_TEST_SUITE(x_tests, TestContext)

//===---------------------------------------------------------------------===//
// Parse xhexb and return the tree.
//===---------------------------------------------------------------------===//

BOOST_AUTO_TEST_CASE(xhexb_run) {
  // Demonstrate the xhexb.x can be parsed into an AST.
  treeXProgram(getXTestPath("xhexb.x"), true);
}

//===---------------------------------------------------------------------===//
// Null programs
//===---------------------------------------------------------------------===//

BOOST_AUTO_TEST_CASE(main_skip) {
  // The simplest program.
  auto program = "proc main () is skip";
  BOOST_TEST(runXProgram(program) == 0);
}

BOOST_AUTO_TEST_CASE(main_stop) {
  // The simplest program.
  auto program = "proc main () is stop";
  BOOST_TEST(runXProgram(program) == 0);
}

//===---------------------------------------------------------------------===//
// Syscalls
//===---------------------------------------------------------------------===//

BOOST_AUTO_TEST_CASE(syscall_exit_0) {
  auto program = R"(
val exit = 0;
proc main () is exit(0)
)";
  BOOST_TEST(runXProgram(program) == 0);
}

BOOST_AUTO_TEST_CASE(syscall_exit_1) {
  auto program = R"(
val exit = 0;
proc main () is exit(1)
)";
  BOOST_TEST(runXProgram(program) == 1);
}

BOOST_AUTO_TEST_CASE(syscall_exit_255) {
  auto program = R"(
val exit = 0;
proc main () is exit(255)
)";
  BOOST_TEST(runXProgram(program) == 255);
}

BOOST_AUTO_TEST_CASE(syscall_exit_neg_255) {
  auto program = R"(
val exit = 0;
proc main () is exit(-255)
)";
  BOOST_TEST(runXProgram(program) == -255);
}

BOOST_AUTO_TEST_CASE(syscall_put_stream_0) {
  auto program = R"(
val put = 1;
proc main () is put('x', 0)
)";
  runXProgram(program);
  BOOST_TEST(simOutBuffer.str() == "x");
}

BOOST_AUTO_TEST_CASE(syscall_put_stream_255) {
  auto program = R"(
val put = 1;
proc main () is put('x', 255)
)";
  runXProgram(program);
  BOOST_TEST(simOutBuffer.str() == "x");
}

BOOST_AUTO_TEST_CASE(syscall_get_stream_0) {
  auto program = R"(
val exit = 0;
val get = 2;
proc main () is exit(get(0))
)";
  simInBuffer.str("a");
  BOOST_TEST(runXProgram(program) == 'a');
}

BOOST_AUTO_TEST_CASE(syscall_get_stream_255) {
  auto program = R"(
val exit = 0;
val get = 2;
proc main () is exit(get(255))
)";
  simInBuffer.str("a");
  BOOST_TEST(runXProgram(program) == 'a');
}

//===---------------------------------------------------------------------===//
// Hello world
//===---------------------------------------------------------------------===//

BOOST_AUTO_TEST_CASE(hello_world_simple) {
  // The most basic hello world example.
  auto program = R"(
val put = 1;
proc main () is {
  put('h', 0);
  put('e', 0);
  put('l', 0);
  put('l', 0);
  put('o', 0);
  put(' ', 0);
  put('w', 0);
  put('o', 0);
  put('r', 0);
  put('l', 0);
  put('d', 0);
  put('\n', 0)
})";
  runXProgram(program);
  BOOST_TEST(simOutBuffer.str() == "hello world\n");
}

BOOST_AUTO_TEST_CASE(hello_world_putval) {
  // Demonstrate a syscall from within a process call.
  auto program = R"(
val put = 1;
proc putval(val c) is put(c, 0)
proc newline() is putval('\n')
proc main() is {
  putval('h');
  putval('e');
  putval('l');
  putval('l');
  putval('o');
  putval(' ');
  putval('w');
  putval('o');
  putval('r');
  putval('l');
  putval('d');
  newline()
})";
  runXProgram(program);
  BOOST_TEST(simOutBuffer.str() == "hello world\n");
}

//===---------------------------------------------------------------------===//
// Procedure calling
//===---------------------------------------------------------------------===//

BOOST_AUTO_TEST_CASE(putval_indirect) {
  // Demonstrate indirection of the character values through a function.
  // Requires the process call to have correctly generated call actuals.
  auto program = R"(
val put = 1;
func foo(val c) is return c
proc putval(val c) is put(c, 0)
proc newline() is putval('\n')
proc main() is {
  putval('x');
  putval(foo('y'));
  newline()
})";
  runXProgram(program);
  BOOST_TEST(simOutBuffer.str() == "xy\n");
}

BOOST_AUTO_TEST_CASE(putval_multiple_indirect) {
  // Demonstrate multiple indirection of the character values through a function.
  auto program = R"(
val put = 1;
func foo(val c) is return c
proc putval(val c) is put(c, 0)
proc newline() is putval('\n')
proc main() is {
  putval(foo('x'));
  putval(foo(foo('y')));
  putval(foo(foo(foo('z'))));
  newline()
})";
  runXProgram(program);
  BOOST_TEST(simOutBuffer.str() == "xyz\n");
}

BOOST_AUTO_TEST_CASE(binop_either_breg) {
  // Materialise RHS value into breg.
  // Only possible when it's a const, string or name.
  auto program = R"(
val put = 1;
func foo(val c) is return c + 1
func bar(val c) is return 1 + c
proc main() is {
  put(foo('a'), 0);
  put(bar('x'), 0);
  put('\n', 0)
})";
  runXProgram(program);
  BOOST_TEST(simOutBuffer.str() == "by\n");
}

BOOST_AUTO_TEST_CASE(binop_func_plus_const) {
  // Binop addition with a function call on LHS and RHS.
  // The '1 + foo()' causes the RHS function call result to be written to the stack.
  auto program = R"(
val put = 1;
func foo(val x) is return x
proc main() is {
  put(foo('a') + 1, 0);
  put(2 + foo('a'), 0);
  put('\n', 0)
})";
  runXProgram(program);
  BOOST_TEST(simOutBuffer.str() == "bc\n");
}

BOOST_AUTO_TEST_CASE(binop_func_args) {
  // Nested binop additions with function calls to supply values, requiring
  // results to be written to the stack.
  auto program = R"(
val put = 1;
func foo(val x) is return x
proc main() is {
  put(foo('a') + foo(1), 0);
  put(foo('a') + foo(1) + foo(1), 0);
  put(foo('a') + foo(1) + foo(1) + foo(1), 0);
  put(foo('a') + foo(1) + foo(1) + foo(1) + foo(1), 0);
  put(foo('a') + foo(1) + foo(1) + foo(1) + foo(1) + foo(1), 0);
  put('\n', 0)
})";
  runXProgram(program);
  BOOST_TEST(simOutBuffer.str() == "bcdef\n");
}

BOOST_AUTO_TEST_CASE(binop_nested_func_args) {
  // Function call bar that must evaluate a binop with a function call.
  auto program = R"(
val put = 1;
func foo(val x) is return x
func bar(val x) is return foo(x) + 1
func baz(val x) is return 1 + foo(x)
proc main() is {
  put(bar('a') + 1, 0);
  put(1 + baz('x'), 0);
  put('\n', 0)
})";
  runXProgram(program);
  BOOST_TEST(simOutBuffer.str() == "cz\n");
}

//===---------------------------------------------------------------------===//
// Arrays
//===---------------------------------------------------------------------===//

BOOST_AUTO_TEST_CASE(array_while_printvals) {
  // Write the contents of an array then print it out.
  // Testing array subscript assignment and access.
  auto program = R"(
array foo[10];
val put = 1;
proc main() is
  var i;
{ i := 0;
  while i < 9 do { foo[i] := i; i:=i+1 };
  i := 0;
  while i < 9 do { put('0'+foo[i], 0); i:=i+1 }
})";
  runXProgram(program);
  BOOST_TEST(simOutBuffer.str() == "012345678");
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

// Semantic errors.

// Enable when (global) arrays are handled.
//BOOST_AUTO_TEST_CASE(semantics_non_const_array_length_error) {
//  auto program = "var x; array foo[x]; proc main () is skip";
//  BOOST_CHECK_THROW(asmXProgram(program), xcmp::NonConstArrayLengthError);
//}

BOOST_AUTO_TEST_SUITE_END()
