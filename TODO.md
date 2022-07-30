# To do list

Features:

- Arithmetic binary and unary operations [DONE]
- Boolean binary and unary operations [DONE]
- Stop statement [DONE]
- If statement [DONE]
- Sequence statement [DONE]
- Assign statement [DONE]
- Global variables [DONE]
- Array variables and subscript elements (LHS and RHS) [DONE]
- String constants [DONE]
- Procedure call as <expr>(...)

Tests:

- Unary operators
- Add, sub operators
- Boolean operators ls, gt, le, ge, not, eq
- Stop statment [DONE]
- If statement
- While statement
- Sequence statement
- Assign statement
- Global variable allocation
- Global array allocation
- Array subscripts (assign and access)
- Array arguments
- Constant pool
- String constants
- Syscalls (exit, read, write) [DONE]

Optimisations:

- x + 0 -> x
- x | 0 -> x
- x - 0 -> x
- ~a | ~b -> ~(a & b)
- ~a & ~b -> ~(a | b)
- Tail recursion