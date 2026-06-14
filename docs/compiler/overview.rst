The compiler
============

The X compiler translates a program written in the :doc:`X language
<../language/overview>` into a binary image for the Hex processor's 8-bit
:doc:`instruction set <../architecture/instruction-set>`. It is deliberately
simple: it performs only a handful of local optimisations, so that the object
code tracks the structure of the source closely and is easy to follow when
reading a trace or disassembly. The generated code is also *position
independent* — branches are encoded as PC-relative offsets and the only
absolute addresses are those of statically-allocated data words — so an image
can be loaded at any base address.

Two implementations
-------------------

There are two compilers for X, which accept the same language:

* ``examples/xhexb.x`` — the original *self-hosting* compiler, written in X
  itself (around 3,000 lines). Because it is written in X it can compile its
  own source; see :doc:`bootstrapping`.

* ``src/xcmp.hpp`` plus ``src/lexer.hpp`` — a modern C++ reimplementation,
  inspired by ``xhexb.x`` and the LLVM Kaleidoscope tutorial. The command-line
  front end is ``tools/xcmp.cpp``.

These reference pages document the C++ implementation in ``src/xcmp.hpp``,
which is the primary compiler used in the project, and note where its
structure differs from the original.

The pipeline
------------

Compilation is driven by ``xcmp::Driver::run`` in ``src/xcmp.hpp``, which
threads the source through a sequence of passes. Each stage has its own
reference page:

#. **Lexical analysis** — ``xcmp::Lexer`` (built on ``hexlex::LexerBase`` in
   ``src/lexer.hpp``) turns source text into a stream of ``xcmp::Token``
   values. See :doc:`lexical-analyser`.

#. **Syntax analysis** — ``xcmp::Parser`` is a recursive-descent parser that
   builds an abstract syntax tree of ``xcmp::AstNode`` subclasses. See
   :doc:`syntax-analyser`.

#. **Semantic passes and optimisation over the tree** — ``CreateSymbols``
   populates a ``SymbolTable``, ``ConstProp`` folds constant expressions, and
   ``OptimiseExpr`` normalises relational and unary operators. See
   :doc:`translator`.

#. **Translation (code generation)** — ``CodeGen`` walks the tree and emits a
   sequence of intermediate directives into a ``CodeBuffer``; ``LowerDirectives``
   resolves frames and calling sequences, and ``OptimiseDirectives`` cleans up
   the instruction stream. See :doc:`translator`, :doc:`codegen-idioms` and
   :doc:`memory-and-calling`.

#. **Assembly and emission** — the directive stream is handed to
   ``hexasm::CodeGen`` (in ``src/hexasm.hpp``), which iteratively resolves
   branch offsets and emits the binary image. See :doc:`codebuffer`.

The same ``Driver`` can stop after any stage to report the intermediate form
(tokens, tree, intermediate instructions, lowered or optimised directives, or
assembly text); these correspond to the ``DriverAction`` values and the
command-line options of ``tools/xcmp.cpp`` (``--tokens``, ``--tree``,
``--insts``, ``-S`` and so on).

When ``main`` is a top-level ``par``, the binary stage instead emits a
*network container* with one image per processor; see :doc:`networks`.
