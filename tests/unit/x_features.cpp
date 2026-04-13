#include "TestContext.hpp"
#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <iostream>
#include <ostream>

//===---------------------------------------------------------------------===//
// Unit tests for X language features.
//===---------------------------------------------------------------------===//

//===---------------------------------------------------------------------===//
// Null programs
//===---------------------------------------------------------------------===//

TEST_CASE("main_skip") {
  TestContext ctx;

  auto program = "proc main () is skip";
  REQUIRE(ctx.runXProgramSrc(program) == 0);
}

TEST_CASE("main_stop") {
  TestContext ctx;

  auto program = "proc main () is stop";
  REQUIRE(ctx.runXProgramSrc(program) == 0);
}

TEST_CASE("stop_after_side_effect") {
  TestContext ctx;

  // Stop after performing a side effect to verify it executes up to stop.
  auto program = R"(
    val put = 1;
    proc main() is { put('a', 0); stop }
  )";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "a");
}

TEST_CASE("stop_in_conditional") {
  TestContext ctx;

  auto program = R"(
    val put = 1;
    proc main() is {
      if true then { put('y', 0); stop } else skip;
      put('n', 0)
    }
  )";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "y");
}

//===---------------------------------------------------------------------===//
// Syscalls
//===---------------------------------------------------------------------===//

TEST_CASE("syscall_exit_0_no_val") {
  TestContext ctx;

  auto program = "proc main() is 0(0)";
  REQUIRE(ctx.runXProgramSrc(program) == 0);
}

TEST_CASE("syscall_exit_0") {
  TestContext ctx;

  auto program = "val exit=0; proc main() is exit(0)";
  REQUIRE(ctx.runXProgramSrc(program) == 0);
}

TEST_CASE("syscall_exit_1") {
  TestContext ctx;

  auto program = "val exit=0; proc main() is exit(1)";
  REQUIRE(ctx.runXProgramSrc(program) == 1);
}

TEST_CASE("syscall_exit_255") {
  TestContext ctx;

  auto program = "val exit=0; proc main() is exit(255)";
  REQUIRE(ctx.runXProgramSrc(program) == 255);
}

TEST_CASE("syscall_exit_neg_255") {
  TestContext ctx;

  auto program = "val exit=0; proc main() is exit(-255)";
  REQUIRE(ctx.runXProgramSrc(program) == -255);
}

TEST_CASE("syscall_put_stream_0_no_val") {
  TestContext ctx;

  auto program = "proc main() is 1('x', 0)";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "x");
}

TEST_CASE("syscall_put_stream_0") {
  TestContext ctx;

  auto program = "val put=1; proc main() is put('x', 0)";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "x");
}

TEST_CASE("syscall_put_stream_255") {
  TestContext ctx;

  auto program = "val put=1; proc main() is put('x', 255)";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "x");
}

TEST_CASE("syscall_get_stream_0_no_vals") {
  TestContext ctx;

  auto program = "proc main() is 0(2(0))";
  REQUIRE(ctx.runXProgramSrc(program, "a") == 'a');
}

TEST_CASE("syscall_get_stream_0") {
  TestContext ctx;

  auto program = "val exit=0; val get=2; proc main() is exit(get(0))";
  REQUIRE(ctx.runXProgramSrc(program, "a") == 'a');
}

TEST_CASE("syscall_get_stream_255") {
  TestContext ctx;

  auto program = "val exit=0; val get=2; proc main() is exit(get(255))";
  REQUIRE(ctx.runXProgramSrc(program, "a") == 'a');
}

TEST_CASE("syscall_echo_multiple") {
  TestContext ctx;

  auto program = R"(
val exit = 0;
val put = 1;
val get = 2;
proc main () is {
  put(get(255));
  put(get(255));
  put(get(255));
  exit(0)
})";
  ctx.runXProgramSrc(program, "abc");
  REQUIRE(ctx.simOutBuffer.str() == "abc");
}

TEST_CASE("syscall_invalid_3") {
  TestContext ctx;

  auto program = "proc main() is 3(0)";
  REQUIRE_THROWS_AS(ctx.runXProgramSrc(program), xcmp::InvalidSyscallError);
}

TEST_CASE("syscall_invalid_val_minus_1") {
  TestContext ctx;

  auto program = "val x=-1; proc main() is x(0)";
  REQUIRE_THROWS_AS(ctx.runXProgramSrc(program), xcmp::InvalidSyscallError);
}

TEST_CASE("syscall_invalid_val_3") {
  TestContext ctx;

  auto program = "val x=3; proc main() is x(0)";
  REQUIRE_THROWS_AS(ctx.runXProgramSrc(program), xcmp::InvalidSyscallError);
}

//===---------------------------------------------------------------------===//
// Hello world
//===---------------------------------------------------------------------===//

TEST_CASE("hello_world_simple") {
  TestContext ctx;

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
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}

TEST_CASE("hello_world_putval") {
  TestContext ctx;

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
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "hello world\n");
}

//===---------------------------------------------------------------------===//
// Constants
//===---------------------------------------------------------------------===//

TEST_CASE("constants_min_positive_pool") {
  TestContext ctx;

  auto program = "val x = 65536; proc main () is 0(x)";
  REQUIRE(ctx.asmXProgramSrc(program, true).str().find("_const0") !=
          std::string::npos);
  REQUIRE(ctx.runXProgramSrc(program) == 65536);
}

TEST_CASE("constants_max_positive_pool") {
  TestContext ctx;

  auto program = "val x = 2147483647; proc main () is 0(x)";
  REQUIRE(ctx.asmXProgramSrc(program, true).str().find("_const0") !=
          std::string::npos);
  REQUIRE(ctx.runXProgramSrc(program) == 2147483647);
}

TEST_CASE("constants_min_negative_pool") {
  TestContext ctx;

  auto program = "val x = -65536; proc main () is 0(x)";
  REQUIRE(ctx.asmXProgramSrc(program, true).str().find("_const0") !=
          std::string::npos);
  REQUIRE(ctx.runXProgramSrc(program) == -65536);
}

TEST_CASE("constants_max_negative_pool") {
  TestContext ctx;

  auto program = "val x = -2147483648; proc main () is 0(x)";
  REQUIRE(ctx.asmXProgramSrc(program, true).str().find("_const0") !=
          std::string::npos);
  REQUIRE(ctx.runXProgramSrc(program) == -2147483648);
}

TEST_CASE("constants_hex") {
  TestContext ctx;

  auto program = "val x = #01000000; proc main () is 0(x)";
  REQUIRE(ctx.runXProgramSrc(program) == 16777216);
}

TEST_CASE("constants_hex_digits") {
  TestContext ctx;

  // Verify hex parsing accepts only valid hex digits (0-9, a-f, A-F).
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(#FF)") == 255);
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(#ff)") == 255);
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(#aB)") == 171);
  // A hex literal followed by a non-hex character should stop at that char,
  // so `#FFg` is parsed as `#FF` followed by identifier `g`.
  REQUIRE(ctx.runXProgramSrc("val g = 0; proc main () is 0(#FF + g)") == 255);
}

TEST_CASE("constants_propagation_binop") {
  TestContext ctx;

  auto program = "val x = 1; val y = 2; val z = 3; val r = (x + y) - z; proc "
                 "main () is 0(r)";
  REQUIRE(ctx.runXProgramSrc(program) == 0);
}

TEST_CASE("local_val_declaration") {
  TestContext ctx;

  auto program = R"(
    func foo() is val x = 42; return x
    proc main() is 0(foo()))";
  REQUIRE(ctx.runXProgramSrc(program) == 42);
}

TEST_CASE("local_val_in_expression") {
  TestContext ctx;

  auto program = R"(
    func bar(val a) is val offset = 10; return a + offset
    proc main() is 0(bar(5)))";
  REQUIRE(ctx.runXProgramSrc(program) == 15);
}

//===---------------------------------------------------------------------===//
// Procedure calling
//===---------------------------------------------------------------------===//

TEST_CASE("putval_indirect") {
  TestContext ctx;

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
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "xy\n");
}

TEST_CASE("putval_multiple_indirect") {
  TestContext ctx;

  // Demonstrate multiple indirection of the character values through a
  // function.
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
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "xyz\n");
}

TEST_CASE("binop_either_breg") {
  TestContext ctx;

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
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "by\n");
}

TEST_CASE("binop_func_plus_const") {
  TestContext ctx;

  // Binop addition with a function call on LHS and RHS.
  // The '1 + foo()' causes the RHS function call result to be written to the
  // stack.
  auto program = R"(
val put = 1;
func foo(val x) is return x
proc main() is {
  put(foo('a') + 1, 0);
  put(2 + foo('a'), 0);
  put('\n', 0)
})";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "bc\n");
}

TEST_CASE("binop_func_args") {
  TestContext ctx;

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
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "bcdef\n");
}

TEST_CASE("binop_nested_func_args") {
  TestContext ctx;

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
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "cz\n");
}

TEST_CASE("three_func_args") {
  TestContext ctx;

  // Nested binop additions with function calls to supply values, requiring
  // results to be written to the stack.
  auto program = R"(
val exit = 0;
func foo(val a0, val a1, val a2) is
  return a0 + a1 + a2
proc main() is exit(foo(0, 1, 2)))";
  REQUIRE(ctx.runXProgramSrc(program) == 3);
}

TEST_CASE("ten_func_args") {
  TestContext ctx;

  // Nested binop additions with function calls to supply values, requiring
  // results to be written to the stack.
  auto program = R"(
val exit = 0;
func foo(val a0, val a1, val a2, val a3, val a4,
         val a5, val a6, val a7, val a8, val a9) is
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9
proc main() is exit(foo(0, 1, 2, 3, 4, 5, 6, 7, 8, 9)))";
  REQUIRE(ctx.runXProgramSrc(program) == 45);
}

TEST_CASE("prepare_call_actuals") {
  TestContext ctx;

  // Check that multiple actuals containing calls are allocated stack space
  // correctly.
  auto program = R"(
func nop(val v) is return v
func add3(val a0, val a1, val a2) is return a0 + a1 + a2
proc main() is 0(add3(nop(1), nop(2), nop(3))))";
  REQUIRE(ctx.runXProgramSrc(program) == 6);
}

TEST_CASE("prepare_call_actuals_nested_1") {
  TestContext ctx;

  // As above, but this time with two levels of nesting.
  auto program = R"(
func nop(val v) is return v
func add3(val a0, val a1, val a2) is return a0 + a1 + a2
proc main() is 0(add3(add3(nop(1), nop(2), nop(3)), nop(4), nop(5))))";
  REQUIRE(ctx.runXProgramSrc(program) == 15);
}

TEST_CASE("prepare_call_actuals_nested_2") {
  TestContext ctx;

  // As above, but this time with three levels of nesting.
  auto program = R"(
func nop(val v) is return v
func add3(val a0, val a1, val a2) is return a0 + a1 + a2
proc main() is 0(add3(add3(add3(nop(1), nop(2), nop(3)), nop(4), nop(5)), nop(6), nop(7))))";
  REQUIRE(ctx.runXProgramSrc(program) == 28);
}

TEST_CASE("prepare_call_actuals_nested_3") {
  TestContext ctx;

  // As above, but this time with the calls nested in an addition.
  auto program = R"(
func nop(val v) is return v
func add3(val a0, val a1, val a2) is return a0 + a1 + a2
proc main() is 0(add3(add3(add3(nop(1)+1, nop(2)+1, nop(3)+1), nop(4)+1, nop(5)+1), nop(6)+1, nop(7)+1)))";
  REQUIRE(ctx.runXProgramSrc(program) == 35);
}

//===---------------------------------------------------------------------===//
// Unary operators.
//===---------------------------------------------------------------------===//

TEST_CASE("unary_minus") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramSrc("proc main () is 0(2(0))", "0") == '0');
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(-(2(0)))", "0") == -'0');
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(-(-(2(0))))", "0") == '0');
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(-(-(-(2(0)))))", "0") == -'0');
  // Constant propagation.
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(-42)") == -42);
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(-(-42))") == 42);
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(-(-(-42)))") == -42);
}

TEST_CASE("unary_not") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramSrc("proc main () is 0(2(0))", "0") == '0');
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(~2(0))", "0") == 0);
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(~(~2(0)))", "0") == 1);
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(~(~(~2(0))))", "0") == 0);
  // With boolean literals.
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(~(2(0)))", {1}) == 0);
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(~(2(0)))", {0}) == 1);
  // Constant propagation.
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(~42)") == 0);
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(~(~42))") == 1);
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(~(~(~42)))") == 0);
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(~true)") == 0);
  REQUIRE(ctx.runXProgramSrc("proc main () is 0(~false)") != 0);
}

//===---------------------------------------------------------------------===//
// Boolean literals
//===---------------------------------------------------------------------===//

TEST_CASE("bool_literal_true") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramSrc("proc main() is if true then 0(1) else 0(2)") ==
          1);
}

TEST_CASE("bool_literal_false") {
  TestContext ctx;

  REQUIRE(ctx.runXProgramSrc("proc main() is if false then 0(1) else 0(2)") ==
          2);
}

TEST_CASE("bool_literal_while_false") {
  TestContext ctx;

  // while false should never execute the body.
  auto program = R"(
    val put = 1;
    proc main() is { while false do put('x', 0); put('y', 0) }
  )";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "y");
}

TEST_CASE("bool_literal_as_func_arg") {
  TestContext ctx;

  auto program = R"(
    func id(val x) is return x
    proc main() is 0(id(true) + id(false)))";
  REQUIRE(ctx.runXProgramSrc(program) == 1);
}

//===---------------------------------------------------------------------===//
// Binary operators.
// Tests using only syscalls and functions.
// Note that the order of evaluation in binary operations is not guaranteed
// left-to-right, so in the tests below where syscalls are used, ordering is
// fixed by performing the calls in separate actuals.
//===---------------------------------------------------------------------===//

TEST_CASE("binary_plus") {
  TestContext ctx;

  auto program = R"(func add(val a, val b) is return a + b
                    proc main () is 0(add(2(0), 2(0))))";
  for (auto a : ctx.getCharValues()) {
    for (auto b : ctx.getCharValues()) {
      REQUIRE(ctx.runXProgramSrc(program, {a, b}) == (a + b));
    }
  }
}

TEST_CASE("binary_sub") {
  TestContext ctx;

  auto program = R"(func sub(val a, val b) is return a - b
                    proc main () is 0(sub(2(0), 2(0))))";
  for (auto a : ctx.getCharValues()) {
    for (auto b : ctx.getCharValues()) {
      REQUIRE(ctx.runXProgramSrc(program, {a, b}) == (a - b));
    }
  }
}

TEST_CASE("binary_sub_rhs_then_lhs") {
  TestContext ctx;

  // A test to show that binops with operatings both requiring aregs are
  // evaluated RHS then LHS (eg b - a in this case).
  auto program = "proc main () is 0(2(0) - 2(0))";
  REQUIRE(ctx.runXProgramSrc(program, {5, 9}) == (9 - 5));
}

TEST_CASE("binary_ls") {
  TestContext ctx;

  auto program = R"(func ls(val a, val b) is return a < b
                    proc main () is 0(ls(2(0), 2(0))))";
  for (auto a : ctx.getCharValues()) {
    for (auto b : ctx.getCharValues()) {
      REQUIRE((ctx.runXProgramSrc(program, {a, b}) != 0) == (a < b));
    }
  }
}

TEST_CASE("binary_ls_rhs_then_lhs") {
  TestContext ctx;

  // Similar to binary_sub_rhs_then_lhs, this was causing a segfault due to the
  // codegen for LS creating an intermediate AST node for the subtraction, then
  // expression generation walking an incomplete AST due to moved unique_ptrs.
  auto program = "proc main () is 0(2(0) < 2(0)))";
  REQUIRE((ctx.runXProgramSrc(program, {5, 9}) != 0) == (9 < 5));
}

TEST_CASE("binary_le") {
  TestContext ctx;

  auto program = R"(func le(val a, val b) is return a <= b
                    proc main () is 0(le(2(0), 2(0))))";
  for (auto a : ctx.getCharValues()) {
    for (auto b : ctx.getCharValues()) {
      REQUIRE((ctx.runXProgramSrc(program, {a, b}) != 0) == (a <= b));
    }
  }
}

TEST_CASE("binary_gt") {
  TestContext ctx;

  auto program = R"(func gt(val a, val b) is return a > b
                    proc main () is 0(gt(2(0), 2(0))))";
  for (auto a : ctx.getCharValues()) {
    for (auto b : ctx.getCharValues()) {
      REQUIRE((ctx.runXProgramSrc(program, {a, b}) != 0) == (a > b));
    }
  }
}

TEST_CASE("binary_ge") {
  TestContext ctx;

  auto program = R"(func ge(val a, val b) is return a >= b
                    proc main () is 0(ge(2(0), 2(0))))";
  for (auto a : ctx.getCharValues()) {
    for (auto b : ctx.getCharValues()) {
      REQUIRE((ctx.runXProgramSrc(program, {a, b}) != 0) == (a >= b));
    }
  }
}

TEST_CASE("binary_eq") {
  TestContext ctx;

  auto program = R"(func eq(val a, val b) is return a = b
                    proc main () is 0(eq(2(0), 2(0))))";
  for (auto a : ctx.getCharValues()) {
    for (auto b : ctx.getCharValues()) {
      REQUIRE((ctx.runXProgramSrc(program, {a, b}) != 0) == (a == b));
    }
  }
}

TEST_CASE("binary_ne") {
  TestContext ctx;

  auto program = R"(func ne(val a, val b) is return a ~= b
                    proc main () is 0(ne(2(0), 2(0))))";
  for (auto a : ctx.getCharValues()) {
    for (auto b : ctx.getCharValues()) {
      REQUIRE((ctx.runXProgramSrc(program, {a, b}) != 0) == (a != b));
    }
  }
}

TEST_CASE("binary_and") {
  TestContext ctx;

  auto program = R"(func and2(val a, val b) is return a and b
                    proc main () is 0(and2(2(0), 2(0))))";
  for (auto a : ctx.getCharValues()) {
    for (auto b : ctx.getCharValues()) {
      REQUIRE((ctx.runXProgramSrc(program, {a, b}) != 0) == (a && b));
    }
  }
}

TEST_CASE("binary_or") {
  TestContext ctx;

  auto program = R"(func or2(val a, val b) is return a or b
                    proc main () is 0(or2(2(0), 2(0))))";
  for (auto a : ctx.getCharValues()) {
    for (auto b : ctx.getCharValues()) {
      REQUIRE((ctx.runXProgramSrc(program, {a, b}) != 0) == (a || b));
    }
  }
}

// Binary operators with constant propagation.

TEST_CASE("binary_add_constants") {
  TestContext ctx;

  for (int a : ctx.getCharValues()) {
    for (int b : ctx.getCharValues()) {
      auto program = fmt::format("proc main () is 0(({}) + ({}))", a, b);
      REQUIRE(ctx.runXProgramSrc(program) == (a + b));
    }
  }
}

TEST_CASE("binary_sub_constants") {
  TestContext ctx;

  for (int a : ctx.getCharValues()) {
    for (int b : ctx.getCharValues()) {
      auto program = fmt::format("proc main () is 0(({}) - ({}))", a, b);
      REQUIRE(ctx.runXProgramSrc(program) == (a - b));
    }
  }
}

TEST_CASE("binary_ls_constants") {
  TestContext ctx;

  for (int a : ctx.getCharValues()) {
    for (int b : ctx.getCharValues()) {
      auto program = fmt::format("proc main () is 0(({}) < ({}))", a, b);
      REQUIRE((ctx.runXProgramSrc(program) != 0) == (a < b));
    }
  }
}

TEST_CASE("binary_le_constants") {
  TestContext ctx;

  for (int a : ctx.getCharValues()) {
    for (int b : ctx.getCharValues()) {
      auto program = fmt::format("proc main () is 0(({}) <= ({}))", a, b);
      REQUIRE((ctx.runXProgramSrc(program) != 0) == (a <= b));
    }
  }
}

TEST_CASE("binary_gt_constants") {
  TestContext ctx;

  for (int a : ctx.getCharValues()) {
    for (int b : ctx.getCharValues()) {
      auto program = fmt::format("proc main () is 0(({}) > ({}))", a, b);
      REQUIRE((ctx.runXProgramSrc(program) != 0) == (a > b));
    }
  }
}

TEST_CASE("binary_ge_constants") {
  TestContext ctx;

  for (int a : ctx.getCharValues()) {
    for (int b : ctx.getCharValues()) {
      auto program = fmt::format("proc main () is 0(({}) >= ({}))", a, b);
      REQUIRE((ctx.runXProgramSrc(program) != 0) == (a >= b));
    }
  }
}

TEST_CASE("binary_eq_constants") {
  TestContext ctx;

  for (int a : ctx.getCharValues()) {
    for (int b : ctx.getCharValues()) {
      auto program = fmt::format("proc main () is 0(({}) = ({}))", a, b);
      REQUIRE((ctx.runXProgramSrc(program) != 0) == (a == b));
    }
  }
}

TEST_CASE("binary_ne_constants") {
  TestContext ctx;

  for (int a : ctx.getCharValues()) {
    for (int b : ctx.getCharValues()) {
      auto program = fmt::format("proc main () is 0(({}) ~= ({}))", a, b);
      REQUIRE((ctx.runXProgramSrc(program) != 0) == (a != b));
    }
  }
}

TEST_CASE("binary_and_constants") {
  TestContext ctx;

  for (int a : ctx.getCharValues()) {
    for (int b : ctx.getCharValues()) {
      auto program = fmt::format("proc main () is 0(({}) and ({}))", a, b);
      REQUIRE((ctx.runXProgramSrc(program) != 0) == (a && b));
    }
  }
}

TEST_CASE("binary_or_constants") {
  TestContext ctx;

  for (int a : ctx.getCharValues()) {
    for (int b : ctx.getCharValues()) {
      auto program = fmt::format("proc main () is 0(({}) or ({}))", a, b);
      REQUIRE((ctx.runXProgramSrc(program) != 0) == (a || b));
    }
  }
}

// Chained associative operators.

TEST_CASE("binary_associative_plus4") {
  TestContext ctx;

  auto program = R"(func add4(val a, val b, val c, val d) is
                      return a + b + c + d
                    proc main () is 0(add4(2(0), 2(0), 2(0), 2(0))))";
  REQUIRE(ctx.runXProgramSrc(program, {1, 2, 3, 4}) == 1 + 2 + 3 + 4);
}

TEST_CASE("binary_associative_and4") {
  TestContext ctx;

  auto program = R"(func and4(val a, val b, val c, val d) is
                      return a and b and c and d
                    proc main () is 0(and4(2(0), 2(0), 2(0), 2(0))))";
  REQUIRE(ctx.runXProgramSrc(program, {0, 0, 0, 0}) == 0);
  REQUIRE(ctx.runXProgramSrc(program, {1, 0, 0, 0}) == 0);
  REQUIRE(ctx.runXProgramSrc(program, {1, 1, 0, 0}) == 0);
  REQUIRE(ctx.runXProgramSrc(program, {1, 1, 1, 0}) == 0);
  REQUIRE(ctx.runXProgramSrc(program, {1, 1, 1, 1}) == 1);
}

TEST_CASE("binary_associative_or4") {
  TestContext ctx;

  auto program = R"(func or4(val a, val b, val c, val d) is
                      return a or b or c or d
                    proc main () is 0(or4(2(0), 2(0), 2(0), 2(0))))";
  REQUIRE(ctx.runXProgramSrc(program, {0, 0, 0, 0}) == 0);
  REQUIRE(ctx.runXProgramSrc(program, {1, 0, 0, 0}) == 1);
  REQUIRE(ctx.runXProgramSrc(program, {0, 1, 0, 0}) == 1);
  REQUIRE(ctx.runXProgramSrc(program, {0, 0, 1, 0}) == 1);
  REQUIRE(ctx.runXProgramSrc(program, {0, 0, 0, 1}) == 1);
  REQUIRE(ctx.runXProgramSrc(program, {1, 1, 1, 1}) == 1);
}

//===---------------------------------------------------------------------===//
// Assign statement
//===---------------------------------------------------------------------===//

TEST_CASE("assign_statement") {
  TestContext ctx;

  auto program = R"(
proc main () is var x; {
  x := 2(0); 0(x)
})";
  REQUIRE(ctx.runXProgramSrc(program, "0") == '0');
  REQUIRE(ctx.runXProgramSrc(program, "1") == '1');
}

TEST_CASE("assign_statement_chained") {
  TestContext ctx;

  auto program = R"(
proc main () is
var x; var y; var z; {
  x := 2(0); y := x; z := y; 0(z)
})";
  REQUIRE(ctx.runXProgramSrc(program, "0") == '0');
  REQUIRE(ctx.runXProgramSrc(program, "1") == '1');
}

//===---------------------------------------------------------------------===//
// If statement
//===---------------------------------------------------------------------===//

TEST_CASE("if_statement") {
  TestContext ctx;

  auto program = "proc main () is if 2(0) = 48 then 0(0) else 0(1)";
  REQUIRE(ctx.runXProgramSrc(program, "0") == 0);
  REQUIRE(ctx.runXProgramSrc(program, "1") == 1);
}

TEST_CASE("if_statement_skip_else") {
  TestContext ctx;

  auto program = "proc main () is if 2(0) = 48 then 0(1) else skip";
  REQUIRE(ctx.runXProgramSrc(program, "0") == 1);
  REQUIRE(ctx.runXProgramSrc(program, "1") == 0);
}

TEST_CASE("if_statement_skip_then") {
  TestContext ctx;

  auto program = "proc main () is if 2(0) = 48 then skip else 0(1)";
  REQUIRE(ctx.runXProgramSrc(program, "0") == 0);
  REQUIRE(ctx.runXProgramSrc(program, "1") == 1);
}

TEST_CASE("if_statement_chained") {
  TestContext ctx;

  auto program = R"(
proc foo(val x) is      if x = 48 then 0(0)
                   else if x = 49 then 0(1)
                   else if x = 50 then 0(2)
                   else 0(3)
proc main () is foo(2(0)))";
  REQUIRE(ctx.runXProgramSrc(program, "0") == 0);
  REQUIRE(ctx.runXProgramSrc(program, "1") == 1);
  REQUIRE(ctx.runXProgramSrc(program, "2") == 2);
  REQUIRE(ctx.runXProgramSrc(program, "3") == 3);
  REQUIRE(ctx.runXProgramSrc(program, "4") == 3);
}

//===---------------------------------------------------------------------===//
// While statement
//===---------------------------------------------------------------------===//

TEST_CASE("while_statement_count") {
  TestContext ctx;

  auto program = R"(
proc main () is
  var i;
{ i := 0;
  while i < 100 do i := i + 1;
  0(i)
})";
  REQUIRE(ctx.runXProgramSrc(program) == 100);
}

TEST_CASE("while_statement_nested_count") {
  TestContext ctx;

  auto program = R"(
proc main () is
  var i;
  var j;
  var count;
{ count := 0;
  i := 0;
  while i < 10 do
  { j := 0;
    while j < 10 do
    { count := count + 1;
      j := j + 1
    };
    i := i + 1
  };
  0(count)
})";
  REQUIRE(ctx.runXProgramSrc(program) == 100);
}

TEST_CASE("while_statement_count_if_exit") {
  TestContext ctx;

  auto program = R"(
proc main () is
  var i;
{ i := 0;
  while true do
  { if i >= 100 then 0(i) else skip;
    i := i + 1
  }
})";
  REQUIRE(ctx.runXProgramSrc(program) == 100);
}

//===---------------------------------------------------------------------===//
// Global variables
//===---------------------------------------------------------------------===//

TEST_CASE("global_var_write_read") {
  TestContext ctx;

  auto program = R"(
    var x;
    proc main() is { x := 42; 0(x) }
  )";
  REQUIRE(ctx.runXProgramSrc(program) == 42);
}

TEST_CASE("global_var_multiple") {
  TestContext ctx;

  auto program = R"(
    var a; var b; var c;
    proc main() is { a := 10; b := 20; c := 30; 0(a + b + c) }
  )";
  REQUIRE(ctx.runXProgramSrc(program) == 60);
}

TEST_CASE("global_var_cross_proc") {
  TestContext ctx;

  // Global variable written in one proc, read in another.
  auto program = R"(
    var x;
    proc set_x(val v) is x := v
    proc main() is { set_x(99); 0(x) }
  )";
  REQUIRE(ctx.runXProgramSrc(program) == 99);
}

TEST_CASE("global_var_func_read") {
  TestContext ctx;

  // Global variable read from a function.
  auto program = R"(
    var x;
    func get_x() is return x
    proc main() is { x := 77; 0(get_x()) }
  )";
  REQUIRE(ctx.runXProgramSrc(program) == 77);
}

//===---------------------------------------------------------------------===//
// Global arrays
//===---------------------------------------------------------------------===//

TEST_CASE("global_array_while_printvals") {
  TestContext ctx;

  // Write the contents of an array then print it out.
  // Testing array subscript assignment and access.
  auto program = R"(
array foo[9];
val put = 1;
proc main() is
  var i;
{ i := 0;
  while i < 10 do
  { foo[i] := i;
    put('0'+foo[i], 0);
    i := i + 1
  }
})";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "0123456789");
}

TEST_CASE("global_array_copy_and_print") {
  TestContext ctx;

  // Exercise generation of array subscripts on LHS and RHS.
  auto program = R"(
    array a[100];
    array b[100];
    val put = 1;
    proc copy (array s, array d, val n) is
      var i;
      var base;
    { base := 0;
      i := 0;
      while i < n do {
        d[i+base] := s[i+base]; | << Important line.
        i := i + 1
      }
    }
    proc print(array a, val n) is
      var i;
    { i := 0;
      while i < n do
      { put(a[i], 0);
        i := i + 1
      };
      put('\n', 0)
    }
    proc main () is
    { a[0] := 'f'; a[1] := 'o'; a[2] := 'o';
      copy(a, b, 3);
      print(b, 3)
    }
  )";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "foo\n");
}

//===---------------------------------------------------------------------===//
// Scope
//===---------------------------------------------------------------------===//

TEST_CASE("scope_local_global_matching") {
  TestContext ctx;

  // Matching variables in local and global scopes.
  auto program = R"(
    var a; var b; var c; var d;
    proc foo(val a, val b, val c) is 1(((a - b) - c) - d, 0)
    proc main() is { a := 42; b := 43; c := 44; d := 3; foo(0, 1, 2) }
  )";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == std::string({-6}));
}

TEST_CASE("scope_matching_formals") {
  TestContext ctx;

  // Matching formal variables in two local scopes, but with different stack
  // offsets.
  auto program = R"(
    proc foo(val a, val b, val c, val d) is 1(((a - b) - c) - d, 0)
    proc bar(val d, val c, val b, val a) is 1(((a - b) - c) - d, 0)
    proc main() is { foo(0, 1, 2, 3); bar(0, 1, 2, 3) }
  )";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == std::string({-6, 0}));
}

TEST_CASE("scope_matching_locals") {
  TestContext ctx;

  // Matching local variables in two local scopes, but with different stack
  // offsets.
  auto program = R"(
    proc foo(val a, val b, val c, val d) is
      var w; var x; var y; var z;
    { w := a; x := b; y := c; z := d;
      1(((w - x) - y) - z, 0)
    }
    proc bar(val a, val b, val c, val d) is
      var z; var y; var x; var w;
    { w := a; x := b; y := c; z := d;
      1(((w - x) - y) - z, 0)
    }
    proc main() is { foo(0, 1, 2, 3); bar(0, 1, 2, 3) }
  )";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == std::string({-6, -6}));
}

//===---------------------------------------------------------------------===//
// Return statement
//===---------------------------------------------------------------------===//

TEST_CASE("return_from_if") {
  TestContext ctx;

  auto program = R"(
    func abs(val x) is if x < 0 then return 0 - x else return x
    proc main() is 0(abs(-5) + abs(5)))";
  REQUIRE(ctx.runXProgramSrc(program) == 10);
}

TEST_CASE("return_from_while") {
  TestContext ctx;

  // Return from inside a while loop.
  auto program = R"(
    func find10() is
      var i;
    { i := 0;
      while true do
      { if i = 10 then return i else skip;
        i := i + 1
      }
    }
    proc main() is 0(find10()))";
  REQUIRE(ctx.runXProgramSrc(program) == 10);
}

TEST_CASE("return_from_nested_if") {
  TestContext ctx;

  auto program = R"(
    func classify(val x) is
      if x < 0 then return 0
      else if x = 0 then return 1
      else return 2
    proc main() is 0(classify(-5) + classify(0) + classify(5)))";
  REQUIRE(ctx.runXProgramSrc(program) == 3);
}

//===---------------------------------------------------------------------===//
// Procs
//===---------------------------------------------------------------------===//

TEST_CASE("proc_no_frame_return") {
  TestContext ctx;

  // Check that procs with no stack allocation return correctly.
  auto program = R"(
    var x; var y;
    proc bar(val z) is if z > y then x := z else skip
    proc main() is { y := 0; bar(42); 0(x) }
  )";
  REQUIRE(ctx.runXProgramSrc(program) == 42);
}

//===---------------------------------------------------------------------===//
// Error handling.
//===---------------------------------------------------------------------===//

// Token errors

TEST_CASE("token_error_char_const_escape") {
  TestContext ctx;

  auto program = "val foo = '\\x';";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::CharConstError);
}

TEST_CASE("token_error_eq") {
  TestContext ctx;

  auto program = "val foo :~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::TokenError);
}

TEST_CASE("token_error_char_const") {
  TestContext ctx;

  auto program = "val foo = 'x~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::TokenError);
}

TEST_CASE("token_error_string") {
  TestContext ctx;

  auto program = "val foo = \"x";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::TokenError);
}

TEST_CASE("token_error_unexpected_char") {
  TestContext ctx;

  auto program = "val foo = ?";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::TokenError);
}

// Unexpected token errors

TEST_CASE("unexpected_token_error_elem") {
  TestContext ctx;

  auto program1 = "val foo = bar[100~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program1), xcmp::UnexpectedTokenError);
  auto program2 = "proc foo() is bar(0~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program2), xcmp::UnexpectedTokenError);
  auto program3 = "val foo = (0~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program3), xcmp::UnexpectedTokenError);
}

TEST_CASE("unexpected_token_error_val_decl") {
  TestContext ctx;

  auto program1 = "val foo ~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program1), xcmp::UnexpectedTokenError);
  auto program2 = "val foo = 1";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program2), xcmp::UnexpectedTokenError);
}

TEST_CASE("unexpected_token_error_array_decl") {
  TestContext ctx;

  auto program1 = "array foo~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program1), xcmp::UnexpectedTokenError);
  auto program2 = "array foo[100~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program2), xcmp::UnexpectedTokenError);
  auto program3 = "array foo[100]~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program3), xcmp::UnexpectedTokenError);
}

TEST_CASE("unexpected_token_error_stmt_if") {
  TestContext ctx;

  auto program1 = "proc foo() is if 0 xxx";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program1), xcmp::UnexpectedTokenError);
  auto program2 = "proc foo() is if 0 then skip xxx";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program2), xcmp::UnexpectedTokenError);
}

TEST_CASE("unexpected_token_error_stmt_while") {
  TestContext ctx;

  auto program = "proc foo() is while 0 xxx";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::UnexpectedTokenError);
}

TEST_CASE("unexpected_token_error_stmt_block") {
  TestContext ctx;

  auto program = "proc foo() is { skip ~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::UnexpectedTokenError);
}

TEST_CASE("unexpected_token_error_stmt_ass") {
  TestContext ctx;

  auto program = "proc foo() is bar ~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::UnexpectedTokenError);
}

TEST_CASE("unexpected_token_error_proc_decl") {
  TestContext ctx;

  auto program0 = "proc foo~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program0), xcmp::UnexpectedTokenError);
  auto program1 = "proc foo(val a ~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program1), xcmp::UnexpectedTokenError);
  auto program2 = "proc foo() ~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program2), xcmp::UnexpectedTokenError);
  auto program3 = "proc foo() is skip x x";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program3), xcmp::UnexpectedTokenError);
}

// Expected name error.

TEST_CASE("expected_name_error") {
  TestContext ctx;

  auto program0 = "proc ~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program0), xcmp::ExpectedNameError);
}

// Parser token errors.

TEST_CASE("parser_token_error_elem") {
  TestContext ctx;

  auto program = "val foo = +";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::ParserTokenError);
}

TEST_CASE("parser_token_error_decl") {
  TestContext ctx;

  auto program = "proc foo() is xxx :=";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::ParserTokenError);
}

TEST_CASE("parser_token_error_formal") {
  TestContext ctx;

  auto program = "proc foo(val a, foo b) is skip";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::ParserTokenError);
}

TEST_CASE("parser_token_error_stmt_invalid") {
  TestContext ctx;

  auto program = "proc foo() is ~";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::ParserTokenError);
}

// Semantic errors.

TEST_CASE("var_formal_parameter") {
  TestContext ctx;

  // Verify `var` formal parameters are parsed and compiled without error.
  auto program = R"(
proc foo(var x) is 0(x)
proc main() is foo(42))";
  REQUIRE(ctx.runXProgramSrc(program) == 42);
}

TEST_CASE("var_formal_parameter_prev_error") {
  TestContext ctx;

  // Before the fix, `var` in a formal position would throw a parser error.
  auto program = "proc foo(var x) is skip proc main() is foo(0)";
  REQUIRE_NOTHROW(ctx.asmXProgramSrc(program));
}

TEST_CASE("var_formal_parameter_func") {
  TestContext ctx;

  // Verify `var` formal parameters work in functions.
  auto program = R"(
    func double(var x) is return x + x
    proc main() is 0(double(21)))";
  REQUIRE(ctx.runXProgramSrc(program) == 42);
}

TEST_CASE("string_expr_parsing") {
  TestContext ctx;

  // Verify that string literals are parsed correctly and not affected by
  // subsequent tokens (regression test for stale lexer string buffer).
  auto program = R"(
val put = 1;
proc putval(val c) is put(c, 0)
proc prints(array s) is skip
proc main() is {
  prints("hello");
  putval('x')
})";
  ctx.runXProgramSrc(program);
  REQUIRE(ctx.simOutBuffer.str() == "x");
}

TEST_CASE("unknown_symbol_error_call") {
  TestContext ctx;

  auto program = "proc main() is foo(0)";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::UnknownSymbolError);
}

TEST_CASE("unknown_symbol_error_expr") {
  TestContext ctx;

  auto program = "proc main() is 0(x)";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program), xcmp::UnknownSymbolError);
}

// Enable when (global) arrays are handled.
TEST_CASE("semantics_non_const_array_length_error") {
  TestContext ctx;

  auto program = "var x; array foo[x]; proc main () is skip";
  REQUIRE_THROWS_AS(ctx.asmXProgramSrc(program),
                    xcmp::NonConstArrayLengthError);
}
