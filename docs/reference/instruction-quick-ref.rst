Instruction quick reference
===========================

A consolidated, at-a-glance table of every Hex instruction and ``OPR``
sub-operation, with its encoded value (in hex) and a one-line effect.  This is
the companion to the prose treatment in :doc:`../architecture/instruction-set`;
the opcode values are those defined in ``src/hex.hpp``.

Each instruction is one byte: a 4-bit opcode in the high nibble and a 4-bit
operand in the low nibble.  Operands larger than four bits — and negative ones —
are built up with chains of ``PFIX`` and ``NFIX`` before the instruction they
modify.

Primary opcodes
---------------

.. list-table:: Opcodes
   :header-rows: 1
   :widths: 14 12 74

   * - Mnemonic
     - Opcode
     - Effect
   * - ``LDAM``
     - ``0x0``
     - Load A from memory: ``areg = mem[oreg]``.
   * - ``LDBM``
     - ``0x1``
     - Load B from memory: ``breg = mem[oreg]``.
   * - ``STAM``
     - ``0x2``
     - Store A to memory: ``mem[oreg] = areg``.
   * - ``LDAC``
     - ``0x3``
     - Load A with a constant: ``areg = oreg``.
   * - ``LDBC``
     - ``0x4``
     - Load B with a constant: ``breg = oreg``.
   * - ``LDAP``
     - ``0x5``
     - Load A with a PC-relative address: ``areg = pc + oreg``.
   * - ``LDAI``
     - ``0x6``
     - Load A from indexed memory: ``areg = mem[breg + oreg]``.
   * - ``LDBI``
     - ``0x7``
     - Load B from indexed memory: ``breg = mem[breg + oreg]``.
   * - ``STAI``
     - ``0x8``
     - Store A to indexed memory: ``mem[breg + oreg] = areg``.
   * - ``BR``
     - ``0x9``
     - Branch PC-relative: ``pc = pc + oreg``.
   * - ``BRZ``
     - ``0xA``
     - Branch if A is zero: ``pc = pc + oreg`` when ``areg == 0``.
   * - ``BRN``
     - ``0xB``
     - Branch if A is negative: ``pc = pc + oreg`` when ``areg < 0``.
   * - ``OPR``
     - ``0xD``
     - Operate: select a register-to-register sub-operation by the operand (see
       below).
   * - ``PFIX``
     - ``0xE``
     - Prefix: ``oreg = oreg << 4``, carrying the operand into the next
       instruction.
   * - ``NFIX``
     - ``0xF``
     - Negative prefix: ``oreg = (~oreg) << 4``, building negative or large
       operands.

OPR sub-operations
------------------

When the opcode is ``OPR`` (``0xD``) the operand selects one of the following
register-to-register operations:

.. list-table:: OPR sub-operations
   :header-rows: 1
   :widths: 14 12 74

   * - Sub-op
     - Operand
     - Effect
   * - ``BRB``
     - ``0x0``
     - Branch to the address in B: ``pc = breg``.
   * - ``ADD``
     - ``0x1``
     - ``areg = areg + breg``.
   * - ``SUB``
     - ``0x2``
     - ``areg = areg - breg``.
   * - ``SVC``
     - ``0x3``
     - Supervisor call: invoke the system call selected by A (see
       :doc:`syscall-reference`).
   * - ``IN``
     - ``0x4``
     - Receive a word into A from the channel link slot selected by B
       (blocking).
   * - ``OUT``
     - ``0x5``
     - Send the word in A to the channel link slot selected by B (blocking).

.. seealso::

   :doc:`../architecture/instruction-set` for the full description of each
   instruction, and :doc:`syscall-reference` for the ``SVC`` system calls.
