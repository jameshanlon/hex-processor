Registers and machine state
===========================

The Hex machine state is intentionally tiny: four 32-bit registers and a flat,
word-addressed memory. There are no condition flags and no general-purpose
register file — every register has a defined role.

The four registers
-------------------

.. list-table::
   :header-rows: 1
   :widths: 12 28 60

   * - Register
     - Name
     - Role
   * - ``pc``
     - Program counter
     - Byte address of the next instruction to fetch.
   * - ``oreg``
     - Operand register
     - Accumulates the immediate operand across ``PFIX``/``NFIX`` prefix chains;
       supplies the operand to the instruction being executed.
   * - ``areg``
     - A register
     - The primary evaluation register: the left operand of an arithmetic
       operation and the place its result is left.
   * - ``breg``
     - B register
     - The secondary evaluation register: the right operand of an arithmetic
       operation.

Why ``oreg`` exists
-------------------

Because an instruction carries only a 4-bit immediate, larger operands have to
be assembled a nibble at a time. ``oreg`` is the register that holds the
operand under construction. Each instruction first merges its own 4-bit
immediate into ``oreg`` with ``oreg = oreg | (inst & 0xf)``, and then uses
``oreg`` as its operand. Most instructions clear ``oreg`` back to ``0`` once
they have used it, so that the next instruction starts a fresh operand.

The ``PFIX`` and ``NFIX`` prefixes are the exception: instead of clearing
``oreg``, they shift it left by four bits, leaving it primed for the next
instruction's nibble. A short run of prefixes therefore builds an arbitrarily
wide immediate before a single operative instruction consumes it. The exact
arithmetic is given in :doc:`instruction-encoding`, and the clear-after-use
behaviour is visible in the execution cycle in :doc:`execution`.

The two evaluation registers
----------------------------

``areg`` and ``breg`` are the machine's two evaluation registers. Having two
distinct load targets — the ``LDAx`` family loads ``areg`` and the ``LDBx``
family loads ``breg`` — lets the compiler set up *both* operands of a binary
operation directly, without first pushing one to the stack. For example, the
two operands of an addition are loaded into ``areg`` and ``breg`` and then
``OPR ADD`` combines them in place. This keeps the common case of a two-operand
expression down to a handful of single-byte instructions.

Memory
------

Memory is word-addressed: each address selects one 32-bit word. Two words at the
base of memory have a fixed meaning:

* **Word 0** holds a branch to the program's entry point, so that execution
  beginning at address 0 jumps straight into the program.
* **Word 1** holds the stack pointer.

The layout of the stack and the calling convention that uses word 1 are
described in :doc:`../compiler/memory-and-calling`.
