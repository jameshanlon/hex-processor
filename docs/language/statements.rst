Statements
==========

A *statement* (or *process*) is a unit of execution. X has a small fixed set of
statement forms, listed here with their meaning. The syntax matches the parser
in ``src/xcmp.hpp`` and the example programs.

``skip``
--------

``skip`` does nothing and terminates immediately. It is most often used as the
mandatory ``else`` arm of a conditional whose alternative has no effect::

   if a[j] > a[j+1] then swap() else skip

``stop``
--------

``stop`` halts the process. Whereas ``skip`` completes successfully and lets
execution continue, ``stop`` does not proceed.

Assignment
----------

::

   variable := expression

evaluates the expression and stores the result in the variable. The
left-hand side may be a simple variable or an array element::

   i := 0
   a[j] := a[j+1]
   div_x := div_x - y

The assignment operator is ``:=`` (a colon immediately followed by an equals
sign); a bare ``=`` is the equality operator in expressions, not assignment.

Sequence
--------

::

   { p ; q ; ... }

runs the statements ``p``, ``q``, ... in order. The braces ``{`` and ``}``
delimit the sequence and the statements are separated by semicolons. By
convention the semicolon may be written at the start of the following line
rather than the end of the preceding one; both styles appear in the examples::

   { i := 0;
     while i < n do step();
     return done() }

Conditional
-----------

::

   if e then p else q

evaluates ``e``; if it is non-zero (true) the statement ``p`` runs, otherwise
``q`` runs. **Both arms are required** — there is no one-armed ``if``. Multi-way
branching is written by nesting conditionals in the ``else`` arm, exactly as in
``fib``::

   if n = 0 then return 0
   else if n = 1 then return 1
   else return fib(n-1) + fib(n-2)

Loop
----

::

   while e do p

evaluates ``e`` and, while it is non-zero (true), runs the statement ``p`` and
re-tests. The loop body is a single statement, usually a brace-delimited
sequence::

   while i < n do
   { sum := sum + a[i];
     i := i + 1
   }

The ``while`` loop is definable in terms of the conditional: ``while e do p`` is
equivalent to "if ``e`` then { ``p``; while ``e`` do ``p`` } else skip". The
compiler emits it directly as a test-and-branch loop rather than by this
expansion; see :doc:`../compiler/codegen-idioms`.

Other statement forms
---------------------

Three further forms are statements but are documented on their own pages because
they belong with larger features:

* A **procedure or syscall call**, ``name(args)`` or ``number(args)`` — see
  :doc:`procedures-functions`.
* A **return**, ``return expression`` — used in function bodies, see
  :doc:`procedures-functions`.
* **Channel input and output**, ``chan ? element`` and ``chan ! expression``,
  and the ``par`` block — see :doc:`concurrency`.
