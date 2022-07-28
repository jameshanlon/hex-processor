# To do list

Features:

- Arithmetic binary and unary operations [DONE, need tests]
- Boolean binary and unary operations [DONE, need tests]
- Stop statement [DONE, need tests]
- If statement [DONE, needs tests]
- Sequence statement [DONE]
- Assign statement [DONE, needs tests]
- Global variables [DONE, needs tests]
- Array variables and subscript elements (LHS and RHS) [DONE, needs tests]
- String constants

Optimisations:

- x + 0 -> x
- x | 0 -> x
- x - 0 -> x
- ~a | ~b -> ~(a & b)
- ~a & ~b -> ~(a | b)
- Tail recursion