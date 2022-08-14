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
- Error on redefined procedure symbol

Tests:

- Stop, skip statments [DONE]
- Syscalls (exit, read, write) [DONE]
- Constant pool [DONE]
- Unary operators -, ~ [DONE]
- Boolean operators +, -, <, <=, >, >=, =, ~= [DONE]
- If statement
- While statement
- Sequence statement
- Assign statement
- Global variable allocation
- Global array allocation
- Array subscripts (assign and access)
- Array arguments
- String constants

Optimisations:

- x + 0 -> x
- x | 0 -> x
- x - 0 -> x
- ~a | ~b -> ~(a & b)
- ~a & ~b -> ~(a | b)
- Tail recursion