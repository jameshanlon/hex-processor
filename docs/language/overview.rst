The X language
==============

X is a small imperative programming language designed to be easy to compile.
It has procedures and functions, the data abstractions ``val``, ``var`` and
``array``, the usual control flow (``if``/``then``/``else`` and ``while``/``do``),
and a message-passing form of concurrency built from ``par``, ``chan`` and the
``!`` / ``?`` channel operators. It compiles to the Hex processor's 8-bit
instruction set (see :doc:`../architecture/instruction-set`).

The language is deliberately minimal. Its compiler, ``xcmp``, is itself
expressible in X — the program :doc:`examples/xhexb.x <examples>` is a working
X compiler written in X — so the whole tool chain can in principle be
bootstrapped on the Hex machine. Keeping the language small is what makes that
practical.

Design choices
--------------

X trades expressive convenience for a grammar and code generator that are easy
to understand and to port:

* **No operator precedence.** Operators do not bind more tightly than one
  another; an expression mixing different operators must be parenthesised, for
  example ``(a + b) < c``. A run of the *same* associative operator may be
  written without brackets, as in ``1 + 2 + 3``.
* **Mandatory ``else``.** Every ``if`` has both a ``then`` and an ``else`` arm;
  there is no one-armed conditional. Use ``else skip`` when the alternative does
  nothing.
* **Pass-by-value scalars.** ``val`` parameters are passed by value. Arrays and
  channels are passed by reference (the callee shares the caller's storage).
* **Single-dimension arrays.** Arrays are one-dimensional and indexed from zero.
* **Procedures and functions, including higher-order ones.** A ``proc`` performs
  a process and returns nothing; a ``func`` computes and returns a value with
  ``return``. Procedures and functions can themselves be passed as ``proc`` and
  ``func`` parameters.
* **Three data abstractions.** ``val`` names a compile-time constant, ``var``
  declares a mutable word-sized variable, and ``array`` declares a block of
  words.

A complete program
------------------

The Fibonacci program is a complete X program. ``main`` reads a number from
input stream ``0`` with the read syscall (``get``, syscall ``2``), computes its
Fibonacci number recursively, and passes the result to the exit syscall
(``exit``, syscall ``0``):

.. literalinclude:: ../../examples/fib.x
   :language: text

A program is a sequence of global declarations followed by procedure and
function definitions; execution begins at ``main``. The numeric syscall
identifiers (``0`` for exit, ``1`` for write, ``2`` for read) are conventionally
bound to ``val`` names such as ``exit``, ``put`` and ``get`` at the top of a
program.

Where to go next
----------------

The remaining pages document each part of the language in detail:

* :doc:`lexical` — the character set, names, literals, comments and escapes.
* :doc:`program-structure` — declarations, abbreviations and scope.
* :doc:`statements` — the process forms (``skip``, assignment, sequence,
  ``if``, ``while``).
* :doc:`expressions` — operands, operators and the no-precedence rule.
* :doc:`procedures-functions` — definitions, formals, calls and ``return``.
* :doc:`concurrency` — ``par``, ``chan`` and message passing.
* :doc:`examples` — a tour of the programs in ``examples/``.
* :doc:`grammar` — the consolidated grammar.
