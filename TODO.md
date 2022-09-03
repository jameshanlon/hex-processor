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
- Error on redefined procedure symbol
- Error on process containing a return statement
- Error on incorrect number of actual parameters
- Error on incorrect types of actual parameters
- Error on string length > 255

Tests:

- Stop, skip statments [DONE]
- Syscalls (exit, read, write) [DONE]
- Constant pool [DONE]
- Unary operators -, ~ [DONE]
- Boolean operators +, -, <, <=, >, >=, =, ~= [DONE]
- If statement [DONE]
- While statement [DONE]
- Sequence statement
- Assign statement [DONE]
- Global variable allocation
- Global array allocation
- Array subscripts (assign and access)
- Array arguments
- String constants
- Constant propagation for expressions
- Scoping of different symbol types
- Reuse binary files from tests compiling the same file
- Resolve 'error near [end] line: illegal character' message with xhexb unit tests

Optimisations:
- Tail recursion (jump to end of EPILOGUE)
- Eliminate BR lab; lab, for procedure exits
- Eliminate
  LDBM 1
  STAI x
  LDAM 1
  LDAI x
- x + 0 -> x
- x | 0 -> x
- x - 0 -> x
- ~a | ~b -> ~(a & b)
- ~a & ~b -> ~(a | b)