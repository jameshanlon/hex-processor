Procedures and functions
========================

X has two kinds of named, callable definition. A **procedure** (``proc``)
carries out a process and returns no value; a **function** (``func``) computes
and yields a value. They share the same definition form and the same parameter
mechanism, differing only in whether they return a result.

Definitions
-----------

A procedure is defined as::

   proc name(formals) is [local declarations] body

and a function identically but with ``func``::

   func name(formals) is [local declarations] body

The parentheses are always present, even when there are no formals
(``proc main() is ...``). After ``is`` a definition may declare local ``val``,
``var`` and ``chan`` names (each terminated by a semicolon) before its single
body statement; that body is usually a brace-delimited sequence::

   proc sort(array a, val n) is
     var i;
     var j;
     var tmp;
   { i := 0;
     while i < n do ...
   }

Formal parameters
-----------------

Each formal is introduced by a keyword that fixes how the argument is passed:

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - Formal
     - Argument
   * - ``val name``
     - A value, passed by value (the callee gets a private copy).
   * - ``array name``
     - An array, passed by reference (callee and caller share the storage).
   * - ``chan name``
     - A channel (see :doc:`concurrency`).
   * - ``proc name``
     - A procedure, passed as a higher-order argument.
   * - ``func name``
     - A function, passed as a higher-order argument.

Because ``proc`` and ``func`` may appear as formals, X is higher-order: a
procedure can take another procedure or function as an argument and call it. The
``globals.x`` example declares ``proc formal_args(val f1, array f2, proc f3,
proc f4)`` to exercise this.

Calls
-----

A call names the callee and supplies a parenthesised, comma-separated list of
actual arguments::

   sort(data, length)
   exit(fib(get(0)))

A call with no arguments still has empty parentheses: ``newline()``. The number
of actuals must match the formals, and each actual must be compatible with its
formal's kind (a value for ``val``, an array for ``array``, and so on). The
meaning of a call is *substitution*: the body runs with each formal standing for
its actual argument.

A numeric call such as ``0(code)`` or ``put(c, 0)`` is a **syscall** — the
callee is a syscall number rather than a name. The conventional ``val`` bindings
are ``exit`` = ``0``, ``put`` = ``1`` and ``get`` = ``2`` (see
:doc:`../architecture/syscalls`).

Functions and ``return``
------------------------

A function returns a value with a ``return`` statement: ``return expression``
evaluates the expression and yields it as the function's result. Control flow
inside a function body may branch, but every path that completes the function
must end in a ``return`` — the final process executed by a function must be a
``return``. The recursive ``fib`` function shows the pattern, with a ``return``
on each arm of the conditional::

   func fib(val n) is
     if n = 0 then return 0
     else if n = 1 then return 1
     else return fib(n-1) + fib(n-2)

A function call is an :doc:`expression <expressions>` and may appear anywhere a
value is expected, including as an argument to another call (``exit(fib(...))``)
or within a larger expression (``fib(n-1) + fib(n-2)``). A procedure call is a
:doc:`statement <statements>` and yields no value.

.. note::

   The original language note described function bodies using a ``valof`` /
   ``return`` form. The current compiler has no ``valof`` keyword: a ``func``
   body is an ordinary statement (optionally preceded by local declarations)
   that produces its result with ``return``, as shown above.

Compilation
-----------

Procedures and functions can be compiled either by *substitution* (inlining the
body at the call site) or as *closed subroutines* (a single shared body entered
by a call and left by a return), with arguments and locals laid out in an
activation frame. How ``xcmp`` lays out frames and links calls is described in
:doc:`../compiler/translator` and :doc:`../compiler/memory-and-calling`.
