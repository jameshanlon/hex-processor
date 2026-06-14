Code-generation idioms
======================

The Hex instruction set is small: it has add and subtract but no multiply,
divide, remainder or bitwise instructions, and it has no instruction that
produces a boolean result. This page describes the recurring patterns the
compiler emits to bridge that gap. They are produced by the nested
``ExprCodeGen`` and ``StmtCodeGen`` visitors of ``CodeBuffer`` in
``src/xcmp.hpp``. The instructions themselves are described in
:doc:`../architecture/instruction-set`, and the source-level operators in
:doc:`../language/expressions`.

Arithmetic
----------

``+`` and ``-`` map directly onto ``ADD`` and ``SUB``. ``genBinopOperands``
materialises the left operand in the A register and the right operand in the B
register, then emits the single ``OPR ADD`` or ``OPR SUB``. When the right
operand is itself a non-trivial expression, it is evaluated first and spilled to
a temporary stack word so that generating the left operand cannot overwrite it.
Recall that ``OptimiseExpr`` has already rewritten unary ``-x`` to ``0 - x``, so
negation is just a subtraction.

Logical ``and`` and ``or``
--------------------------

The logical operators are short-circuiting and are implemented with conditional
branches rather than as values. For ``a and b`` the compiler evaluates ``a`` and
emits ``BRZ end``; if ``a`` is zero the result (still in the A register) is
false, otherwise control falls through and ``b`` is evaluated to give the
result. For ``a or b`` it evaluates ``a``, and if non-zero branches past the
evaluation of ``b``; otherwise ``b`` supplies the result. Unary ``~`` is
generated as a branch that produces ``LDAC 0`` or ``LDAC 1``.

Comparisons
-----------

Recall from :doc:`translator` that ``OptimiseExpr`` reduces all six relational
operators to combinations of ``<``, ``=`` and ``~``, so code generation only
implements two comparisons directly.

For ``a = b`` the compiler computes ``a - b`` and tests it for zero. As a
special case, comparing against zero skips the subtraction. It then materialises
a boolean with a ``BRZ`` and two ``LDAC`` arms:

.. literalinclude:: ../../src/xcmp.hpp
   :language: cpp
   :lines: 2627-2651

For ``a < b`` it computes ``a - b`` and tests the sign with ``BRN`` (again,
comparing against zero skips the subtraction since the sign of ``a`` is enough):

.. literalinclude:: ../../src/xcmp.hpp
   :language: cpp
   :lines: 2652-2676

Comparison in a condition
-------------------------

Materialising a 0/1 boolean is only necessary when the comparison's *value* is
used. When a comparison appears directly as the condition of an ``if`` or
``while``, the surrounding statement generation emits its own ``BRZ`` against
the condition, so the value is consumed without ever building the explicit
boolean. The peephole pass ``OptimiseDirectives`` further cleans up the
generated branches (for example dropping a ``BR`` to the next instruction), so
conditionals are tight in practice.

Multiplication, division and remainder
--------------------------------------

Because there are no multiply, divide or bitwise instructions, these operations
are not generated inline. They are written as ordinary X library procedures
built from add, subtract and the comparison idioms above, and called like any
other function. The example programs include shift-and-add multiplication in
``examples/mul.x`` (and ``examples/mul2.x``) and long division in
``examples/div.x`` (which also yields the remainder). A program that needs these
operations includes the corresponding routine and calls it.
