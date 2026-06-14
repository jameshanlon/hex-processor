Program structure
=================

An X program is a sequence of **global declarations** followed by a sequence of
**procedure and function definitions**::

   <global declarations>
   <procedure and function definitions>

Execution begins at the procedure named ``main``. There is no separate
"top-level process" — the program *is* its definitions, and ``main`` is the one
that runs. The Fibonacci program in :doc:`overview` is a minimal example: two
``val`` declarations, then ``proc main`` and ``func fib``.

Declarations
------------

A declaration introduces a name and gives its meaning for the rest of the
enclosing scope.

``val name = expression``
   A *constant abbreviation*: ``name`` stands for the value of the constant
   expression. Conventionally used for syscall numbers and other named
   constants, for example ``val put = 1;`` or ``val length = 10;``. The
   right-hand side is evaluated at compile time, so it may itself refer to
   earlier ``val`` names: ``val c2 = 1 + 2 + 3;``.

``var name``
   Declares a mutable, word-sized variable. A fresh global ``var`` is zero; a
   local ``var`` is uninitialised until first assigned.

``array name[expression]``
   Declares a one-dimensional array of the given (constant) number of words,
   indexed from zero. The size expression may use earlier ``val`` names, e.g.
   ``array data[length];``. Arrays may only be declared at global scope.

``chan name``
   Declares a channel for message-passing concurrency. See :doc:`concurrency`.

Each declaration is terminated by a semicolon. Global declarations may be
``val``, ``var``, ``array`` or ``chan``; the ``globals.x`` example exercises all
the constant-expression forms of array sizing.

Scope and abbreviation
----------------------

A declaration is in scope from the point it appears to the end of the program
(for globals) or to the end of the enclosing procedure (for locals). The meaning
of an abbreviation is *substitution*: wherever the abbreviated name appears, it
denotes the thing it was bound to. For a ``val``, every use of the name behaves
as though the constant value had been written in its place; this is why ``val``
right-hand sides must be constant expressions.

Procedures and functions are likewise named definitions whose names may be used
(as calls, or passed as ``proc``/``func`` arguments) anywhere in scope. They are
described in full on the :doc:`procedures-functions` page. A procedure may
declare its own local ``val``, ``var`` and ``chan`` declarations between ``is``
and its body; these are local abbreviations following the same substitution
rule.
