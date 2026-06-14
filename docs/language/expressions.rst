Expressions
===========

An *expression* computes a value. Expressions are built from operands combined
by monadic (unary) and diadic (binary) operators. The forms below match the
expression parser in ``src/xcmp.hpp``.

Operands and elements
----------------------

The simplest expressions are *elements*:

* a **variable** reference, ``name``;
* an **array element**, ``name[subscript]``, where the subscript is itself an
  expression and indexing is from zero;
* an **integer literal**, ``42`` or ``#ff``; a **character constant**, ``'P'``;
  a **string**, ``"hi"``; or the boolean literals ``true`` and ``false``;
* a **function or syscall call**, ``name(args)`` or ``number(args)``, which is
  an expression yielding the call's result;
* a **parenthesised expression**, ``( expression )``, used both for grouping and
  (with no operators between) wherever a sub-expression is wanted.

Array elements are read and written through the same ``name[subscript]`` syntax;
as an expression it reads the element, and as the left side of ``:=`` it writes
it.

Monadic operators
-----------------

Two prefix operators apply to a single element:

* ``-`` negates: ``-n`` is ``0 - n`` (arithmetic is signed).
* ``~`` is logical/bitwise *not*: ``~e`` inverts ``e``. It is written ``~``, not
  a ``not`` keyword. It appears in the examples as ``~lsu(x, y)``.

Diadic operators
----------------

The binary operators are:

.. list-table::
   :header-rows: 1
   :widths: 24 18 58

   * - Category
     - Operators
     - Meaning
   * - Arithmetic
     - ``+`` ``-``
     - Signed addition and subtraction.
   * - Logical
     - ``and`` ``or``
     - Logical/bitwise conjunction and disjunction (keywords).
   * - Relational
     - ``=`` ``~=`` ``<`` ``<=`` ``>`` ``>=``
     - Equal, not-equal, and the four orderings.

Note the spellings: not-equal is ``~=`` (not ``<>``), and the boolean
connectives ``and`` / ``or`` are reserved words while negation uses the ``~``
operator. There is no division or multiplication operator built into the
language — programs needing them call helper functions such as the ``div`` /
``rem`` pair found in ``gcd.x`` and several other examples.

No precedence, and associativity
--------------------------------

X has **no operator precedence**: no operator binds more tightly than any other.
An expression that mixes *different* operators must be parenthesised to make the
grouping explicit. For instance ``(x < 0) = (y < 0)`` and ``(n + n + n) + 1``
both appear in the examples with brackets that a precedence-based language would
let you omit. A run of the *same* associative operator, however, may be chained
without brackets — ``1 + 2 + 3`` and ``a and b and c`` are accepted directly.

Because relational and arithmetic operators sit at the same level, a comparison
used as a value (as in ``return x < y``) and a comparison used as a condition
(``while i < n do ...``) are the same kind of expression: a non-zero result is
true and zero is false.

Lowering
--------

How comparisons and boolean expressions are turned into the Hex processor's
conditional branches (``BRZ`` / ``BRN``) is covered in
:doc:`../compiler/codegen-idioms`.
