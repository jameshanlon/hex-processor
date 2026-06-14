Bootstrapping
=============

The original X compiler, ``examples/xhexb.x``, is written in X itself — around
3,000 lines (the file is 3,053 lines). Because it is an X program, it can be
compiled to a Hex image and then used to compile X programs, including its own
source. This self-hosting property is the practical demonstration that X is
expressive enough to write a non-trivial program: a complete lexer, parser, tree
builder and code generator, all in the same language the compiler accepts.

The bootstrap chain
-------------------

Self-hosting raises the usual chicken-and-egg question: you need an X compiler
to compile the X compiler. The project breaks the cycle with a hand-prepared
assembly image of the compiler, ``tests/asm/xhexb.S`` (transcribed from the
original published assembly listing). The chain is:

#. Assemble ``tests/asm/xhexb.S`` with the assembler ``hexasm`` to produce the
   compiler image ``xhexb.bin``.
#. Run ``xhexb.bin`` under the simulator ``hexsim``, feeding an X program on
   standard input. The compiler reads the source from stdin and writes the
   compiled binary (the integration tests collect it as ``simout2``).
#. In particular, feeding ``examples/xhexb.x`` to ``xhexb.bin`` compiles the
   compiler with itself, closing the loop.

This is exactly what the integration tests in ``tests/tests.py`` do: ``setUp``
assembles ``xhexb.bin`` from the ``.S`` source, the per-program tests pipe an X
file into it, and ``test_x_compiler_sim`` pipes ``xhexb.x`` itself in and checks
the reported tree size, program size and image size. The same test is repeated
against the RTL via the Verilator testbench ``hextb``.

In rough form, the steps look like::

   # Build the seed compiler image from the hand-assembled source.
   hexasm tests/asm/xhexb.S -o xhexb.bin

   # Use it to compile an X program (source on stdin, image on stdout).
   hexsim xhexb.bin < examples/xhexb.x

The original compiler's structure mirrors the stages documented elsewhere in
this section: the lexer procedures (``rdline``, ``rch``, ``readnumber``,
``readstring`` and ``nextsymbol``; see :doc:`lexical-analyser`), the tree
constructors ``cons1`` … ``cons4`` that build the tagged-vector AST (see
:doc:`syntax-analyser`), and the translator that walks that tree to emit Hex
instructions.

The C++ implementation
----------------------

Alongside the self-hosting compiler, the project provides a parallel
reimplementation in C++: ``src/xcmp.hpp`` (with ``src/lexer.hpp``) and the
command-line front end ``tools/xcmp.cpp``, built as the ``xcmp`` tool. It
accepts the same language and produces compatible images, but is an ordinary
host program — it does not require the bootstrap chain — and it is the compiler
these reference pages describe in detail. The two compilers are useful as a
cross-check on each other.

The tools used above (``hexasm``, ``hexsim``, ``xcmp``, ``hextb`` and the
end-to-end runner ``xrun``) are described in :doc:`../tools/index`.
