# To do list

Features:

- Arithmetic binary and unary operations [DONE, need tests]
- Boolean binary and unary operations [DONE, need tests]
- Stop statement [DONE, need tests]
- If statement
- Sequence statement
- Assign statement
- Global variables
- Array variables and subscript elements
- String constants

Optimisations:

- x + 0 -> x
- x | 0 -> x
- x - 0 -> x
- ~a | ~b -> ~(a & b)
- ~a & ~b -> ~(a | b)
- Tail recursion