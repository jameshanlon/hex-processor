Instruction set
===============

The instruction set is organised into a handful of groups. Each instruction is
a single byte whose high nibble is the opcode listed below and whose low nibble
contributes to the operand ``oreg`` (see :doc:`instruction-encoding`). In the
effect column, ``oreg`` denotes the fully assembled operand and ``mem[w]`` the
word at word-address ``w``.

Data access
-----------

.. list-table::
   :header-rows: 1
   :widths: 12 12 28 48

   * - Mnemonic
     - Opcode
     - Effect
     - Description
   * - ``LDAM``
     - ``0x0``
     - ``areg = mem[oreg]``
     - Load ``areg`` from the memory word at address ``oreg``.
   * - ``LDBM``
     - ``0x1``
     - ``breg = mem[oreg]``
     - Load ``breg`` from the memory word at address ``oreg``.
   * - ``STAM``
     - ``0x2``
     - ``mem[oreg] = areg``
     - Store ``areg`` to the memory word at address ``oreg``.

Constants and program addresses
-------------------------------

.. list-table::
   :header-rows: 1
   :widths: 12 12 28 48

   * - Mnemonic
     - Opcode
     - Effect
     - Description
   * - ``LDAC``
     - ``0x3``
     - ``areg = oreg``
     - Load ``areg`` with the constant operand.
   * - ``LDBC``
     - ``0x4``
     - ``breg = oreg``
     - Load ``breg`` with the constant operand.
   * - ``LDAP``
     - ``0x5``
     - ``areg = pc + oreg``
     - Load ``areg`` with a PC-relative address (the operand added to the
       current ``pc``).

Indexed data structures
------------------------

.. list-table::
   :header-rows: 1
   :widths: 12 12 32 44

   * - Mnemonic
     - Opcode
     - Effect
     - Description
   * - ``LDAI``
     - ``0x6``
     - ``areg = mem[areg + oreg]``
     - Load ``areg`` from memory indexed by ``areg`` plus the operand.
   * - ``LDBI``
     - ``0x7``
     - ``breg = mem[breg + oreg]``
     - Load ``breg`` from memory indexed by ``breg`` plus the operand.
   * - ``STAI``
     - ``0x8``
     - ``mem[breg + oreg] = areg``
     - Store ``areg`` to memory indexed by ``breg`` plus the operand.

These indexed forms make array and structure access cheap: the base address sits
in a register and the operand supplies the field or element offset.

Branch, jump and call
---------------------

.. list-table::
   :header-rows: 1
   :widths: 12 16 28 44

   * - Mnemonic
     - Opcode
     - Effect
     - Description
   * - ``BR``
     - ``0x9``
     - ``pc = pc + oreg``
     - Unconditional PC-relative branch.
   * - ``BRZ``
     - ``0xA``
     - ``if areg == 0: pc = pc + oreg``
     - Branch if ``areg`` is zero.
   * - ``BRN``
     - ``0xB``
     - ``if areg < 0: pc = pc + oreg``
     - Branch if ``areg`` is negative (signed).
   * - ``BRB``
     - ``OPR 0x0``
     - ``pc = breg``
     - Branch to the absolute address held in ``breg`` (an ``OPR`` sub-op, see
       below).

Procedure calls are built from these primitives rather than from a dedicated
call instruction. The caller uses ``LDAP`` to compute the return address (the
address just past the call) into ``areg`` and a ``BR`` to jump to the callee's
entry point. The callee stores the return address it was handed, runs its body,
loads that return address back into ``breg``, and returns with ``BRB``, which
jumps to the absolute address in ``breg``. The full stack frame layout and
register usage are described in :doc:`../compiler/memory-and-calling`.

Expression operations (``OPR``)
-------------------------------

The ``OPR`` opcode (``0xD``) does not take an immediate in the usual sense:
instead its assembled operand selects an inter-register operation. The operand
values are:

.. list-table::
   :header-rows: 1
   :widths: 12 14 30 44

   * - Sub-op
     - Operand
     - Effect
     - Description
   * - ``BRB``
     - ``0x0``
     - ``pc = breg``
     - Branch to the address in ``breg`` (used for returns and computed jumps).
   * - ``ADD``
     - ``0x1``
     - ``areg = areg + breg``
     - Add ``breg`` to ``areg``.
   * - ``SUB``
     - ``0x2``
     - ``areg = areg - breg``
     - Subtract ``breg`` from ``areg``.
   * - ``SVC``
     - ``0x3``
     - supervisor call
     - Perform a system call selected by ``areg`` (see :doc:`syscalls`).
   * - ``IN``
     - ``0x4``
     - ``areg = `` channel ``breg``
     - Receive a word from the channel on link slot ``breg`` (blocking).
   * - ``OUT``
     - ``0x5``
     - channel ``breg`` ``= areg``
     - Send the word in ``areg`` over the channel on link slot ``breg``
       (blocking).

The channel operations ``IN`` and ``OUT`` perform a synchronous rendezvous and
are described in detail in :doc:`channels`.

Prefixes
--------

.. list-table::
   :header-rows: 1
   :widths: 12 12 36 40

   * - Mnemonic
     - Opcode
     - Effect
     - Description
   * - ``PFIX``
     - ``0xE``
     - ``oreg = oreg << 4``
     - Positive prefix: shift the accumulated operand up by a nibble.
   * - ``NFIX``
     - ``0xF``
     - ``oreg = 0xFFFFFF00 | (oreg << 4)``
     - Negative prefix: shift up and sign-extend, for negative or large operands.

The prefix arithmetic and a worked example are given in
:doc:`instruction-encoding`.

.. note::

   For a compact one-page summary of every opcode and ``OPR`` sub-op, see
   :doc:`../reference/instruction-quick-ref`.
