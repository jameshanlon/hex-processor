Processor core
==============

The processor datapath lives in ``rtl/processor.sv``. It implements the abstract
execution cycle of :doc:`../architecture/execution` directly: each clock edge
fetches one instruction, decodes it, and updates the architectural registers.
There is no pipeline — every register is updated combinationally from the
current instruction and the current register values, and the whole next state is
committed on the rising clock edge.

Registers and state
-------------------

The processor holds exactly the four architectural registers of
:doc:`../architecture/registers`, each as a flip-flop with a combinational
"next value":

.. list-table::
   :header-rows: 1
   :widths: 20 25 55

   * - Register
     - State / next
     - Role
   * - PC
     - ``pc_q`` / ``pc_d``
     - Program counter (a byte address, ``MEM_ADDR_WIDTH`` = 21 bits).
   * - AREG
     - ``areg_q`` / ``areg_d``
     - Accumulator and ALU result; also the value sent on ``OUT`` and the
       received value on ``IN``.
   * - BREG
     - ``breg_q`` / ``breg_d``
     - Second ALU operand and index register; its low bits select the channel
       slot on a channel op.
   * - OREG
     - ``oreg_q`` / ``oreg_d``
     - Operand register accumulated by ``PFIX`` / ``NFIX``.

All four are reset to zero and, on every non-stalled cycle, take their ``_d``
values:

.. literalinclude:: ../../rtl/processor.sv
   :language: verilog
   :lines: 48-59

Instruction fetch and decode
----------------------------

Fetch is trivial: the processor drives ``o_f_addr = pc_q`` with ``o_f_valid``
permanently high, and the memory returns the addressed byte on ``i_f_data``. The
byte is typed as ``hex_pkg::instr_t``, a packed struct of a 4-bit ``opcode`` and
a 4-bit ``operand``, so decode is just field extraction:

.. literalinclude:: ../../rtl/processor.sv
   :language: verilog
   :lines: 61-69

The shared opcode values come from ``rtl/hex_pkg.sv``, which is the single source
of truth for the encoding documented in :doc:`../architecture/instruction-set`:

.. literalinclude:: ../../rtl/hex_pkg.sv
   :language: verilog
   :lines: 27-58

These enumerators carry exactly the opcode numbers in the instruction-set
reference — ``LDAM = 0x0`` … ``NFIX = 0xF`` — so the RTL ``unique case``
statements over ``instr.opcode`` correspond directly to the table there. The
register-to-register operations live behind ``OPR`` (``0xD``) and are selected by
the operand via ``opr_opcode_t`` (``ADD``, ``SUB``, ``SVC``, ``IN``, ``OUT``).
``instr_svc`` / ``instr_in`` / ``instr_out`` are decoded as ``OPR`` with the
matching operand.

The operand register and PFIX/NFIX
----------------------------------

Every instruction's effective operand is ``opr_d = oreg_q | instr.operand`` — the
accumulated prefix bits OR'd with the current instruction's 4-bit operand field.
``PFIX`` updates ``oreg_d = opr_d << 4`` to shift the accumulated value up a
nibble, and ``NFIX`` does the same but with the sign bits set
(``32'hFFFFFF00 | (opr_d << 4)``) so that negative immediates can be built. For
any non-prefix instruction ``oreg_d`` is cleared back to zero, so the operand
accumulation is consumed by the instruction that follows the prefix chain. This
is the hardware realisation of the PFIX/NFIX scheme in
:doc:`../architecture/instruction-encoding`.

The A/B multiplexors and the ALU
--------------------------------

Each register's next value is a multiplexor over the opcode. ``areg_d`` is the
busiest: it selects between a data-memory word (``LDAM`` / ``LDAI``), the operand
(``LDAC``), a PC-relative address (``LDAP``), the received channel word
(``IN``), and the ALU outputs for ``ADD`` / ``SUB``:

.. literalinclude:: ../../rtl/processor.sv
   :language: verilog
   :lines: 108-125

The ALU is just the ``areg_q + breg_q`` and ``areg_q - breg_q`` adders inside
this multiplexor — there is no separate ALU block. ``breg_d`` is a smaller mux
(``LDBM`` / ``LDBI`` from memory, ``LDBC`` from the operand), and ``pc_d``
defaults to ``pc_q + 1`` but is overridden by the branch opcodes (``BR``,
``BRZ``, ``BRN``, and ``OPR BRB`` which jumps to ``breg``).

Memory and syscall interface
----------------------------

The data-memory port is driven combinationally from the decode. ``o_d_valid`` is
asserted for the load/store opcodes, ``o_d_we`` for the two stores
(``STAM`` / ``STAI``), and ``o_d_addr`` is computed per opcode — a direct operand
address for the ``*M`` forms and a base-plus-offset (``areg``/``breg`` plus
operand) for the indexed ``*I`` forms. Stores always present ``areg_q`` on
``o_d_data``. Supervisor calls are surfaced by asserting ``o_syscall_valid`` on
``instr_svc`` with ``o_syscall = syscall_t'(areg_q)``; the testbench services the
call (see :doc:`../architecture/syscalls`).

Channel operations and the stall path
-------------------------------------

``IN`` and ``OUT`` are the only instructions that can take more than one cycle.
The processor hands the operation off to the per-core link interface and freezes
while the rendezvous completes:

.. literalinclude:: ../../rtl/processor.sv
   :language: verilog
   :lines: 71-78

The channel slot is taken from the low ``SLOT_W`` bits of ``breg_q`` and the word
to send from ``areg_q``. While the link interface reports ``i_liu_busy``, the
processor asserts ``stall``, which gates the state-register update shown above so
that *all* of PC/AREG/BREG/OREG hold their values. When the link interface
completes, ``stall`` drops; on ``IN`` the received word reaches ``areg`` via the
``areg_d = i_liu_in_word`` mux arm, and the default ``pc_d = pc_q + 1`` then
advances past the instruction. The mechanics of the rendezvous itself are
covered in :doc:`memory-and-links`, and the channel semantics in
:doc:`../architecture/channels`.

In the single-core ``hex`` top (``rtl/hex.sv``) these channel ports are tied off
— ``i_liu_busy`` is held low and the outputs left unconnected — so a sequential
image never stalls on a channel op.
