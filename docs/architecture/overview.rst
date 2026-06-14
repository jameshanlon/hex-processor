The Hex architecture
====================

Purpose
-------

Hex is a deliberately minimal processor designed to explain how a computer
works. It is small enough to implement in hardware, yet flexible enough to
execute substantial programs — including a self-hosting compiler for the
:doc:`X language <../language/overview>`. The architecture has four 32-bit
registers and a single-byte instruction format, so the whole datapath and
instruction decoder can be understood in their entirety, and the same model
serves both as a pedagogical example and as a working target for real tools.

Design principles
-----------------

The architecture follows a few simple rules that keep both the instruction
encoding and the hardware small:

* **Short instructions** give efficient access to the stack and data regions.
  The common operations — loading constants, loading and storing locals,
  branching — each fit in a single byte.
* **Word-addressed memory, single-byte instructions.** Data is addressed a word
  at a time, but instructions are one byte each. An instruction address is
  therefore a byte position *within* a word: the low bits of the program counter
  select a byte inside the word the high bits address.
* **Word-length independence.** Because instruction addresses are byte positions
  within a word, the same instruction set works for any word length that is a
  whole number of bytes.
* **Few registers, some special-purpose.** There are only four registers, and
  most have a fixed role (see :doc:`registers`), which keeps the encoding dense
  and the decode logic trivial.
* **Easy-to-decode instructions.** Every instruction has the same fixed shape, so
  decoding is a matter of splitting one byte into two nibbles.

The instruction format in brief
-------------------------------

Every instruction is exactly 8 bits wide: a 4-bit operation in the high nibble
and a 4-bit immediate in the low nibble. The immediate alone covers the values
0–15, which is enough for the great majority of constants, offsets, and branch
distances in real code.

Two mechanisms extend this compact format:

* ``OPR`` reinterprets its 4-bit operand not as an immediate but as a selector
  for an inter-register operation (addition, subtraction, branch-to-register,
  supervisor calls, and channel transfers).
* ``PFIX`` and ``NFIX`` are *prefix* instructions that shift accumulated nibbles
  into a wider operand, so any immediate — however large, positive or negative —
  can be built from a short chain of bytes.

The full encoding, including the prefix arithmetic, is described in
:doc:`instruction-encoding`.

Lineage
-------

Hex is descended from the Transputer and from the *Simple 42* teaching
processor, inheriting their stack-oriented, prefix-extended instruction style
and their channel-based communication model.

See also
--------

* :doc:`registers` — the machine state and the role of each register.
* :doc:`instruction-set` — the full instruction set, grouped by function.
* :doc:`execution` — the fetch–increment–execute cycle and the datapath.

.. toctree::
   :hidden:

   registers
   instruction-encoding
   instruction-set
   execution
   channels
   syscalls
   simulator-model
