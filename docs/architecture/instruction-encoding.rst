Instruction encoding
====================

Every Hex instruction is a single byte. The high nibble selects the operation
and the low nibble carries a 4-bit immediate operand:

.. code-block:: text

   bit:   7 6 5 4 3 2 1 0
         +-------+-------+
         | oper  | imm   |
         +-------+-------+

The immediate field can directly express the values 0–15. Anything outside that
range — larger offsets, larger constants, and all negative values — is built up
using the prefix instructions described below.

Operand accumulation
--------------------

Operands are assembled in the operand register ``oreg``. Before executing, every
instruction folds its own immediate into ``oreg``::

   oreg = oreg | (inst & 0xf)

It then uses ``oreg`` as its operand. Most instructions clear ``oreg`` back to
``0`` afterwards, so that an instruction with no preceding prefixes simply sees
its own 4-bit immediate and the next instruction starts clean.

The prefix instructions ``PFIX`` and ``NFIX`` are the exception: rather than
clearing ``oreg``, they shift the accumulated nibbles up to make room for the
next instruction's nibble.

``PFIX`` — positive prefix
~~~~~~~~~~~~~~~~~~~~~~~~~~~

``PFIX`` shifts ``oreg`` left by four bits and continues to the next
instruction::

   oreg = oreg << 4

This concatenates the prefix's nibble with whatever nibble the following
instruction contributes, building up a positive value four bits at a time.

``NFIX`` — negative prefix
~~~~~~~~~~~~~~~~~~~~~~~~~~~

``NFIX`` shifts left by four bits as well, but also fills the high bits with
ones::

   oreg = 0xFFFFFF00 | (oreg << 4)

This sign-extends the operand, and is used to build negative or large-magnitude
immediates.

Worked example: loading a constant larger than 15
-------------------------------------------------

Suppose we want to load the constant ``0x2A`` (decimal 42) into ``areg``. It does
not fit in a single nibble, so the assembler emits one ``PFIX`` followed by the
``LDAC``:

.. list-table::
   :header-rows: 1
   :widths: 16 20 64

   * - Byte
     - Mnemonic
     - Effect on ``oreg``
   * - ``0xE2``
     - ``PFIX 2``
     - ``oreg = oreg | 0x2`` → ``0x2``; then ``oreg = 0x2 << 4`` → ``0x20``
   * - ``0x3A``
     - ``LDAC 10``
     - ``oreg = 0x20 | 0xA`` → ``0x2A``; then ``areg = oreg`` → ``0x2A``

The first nibble (``2``) is shifted up by ``PFIX`` to occupy bits 7–4, and the
second nibble (``A``) of ``LDAC`` fills bits 3–0, giving ``0x2A``. Larger
constants chain more ``PFIX`` bytes, four bits per prefix; a negative constant
begins the chain with an ``NFIX`` so the high bits are filled with ones.

Prefixes are inserted automatically
-----------------------------------

Programmers and code generators do not emit prefixes by hand. The assembler and
compiler compute the minimal prefix chain needed for each operand. Because a
branch distance depends on the size of the instructions between the branch and
its target — which in turn depends on how many prefixes those instructions need —
the encoding is resolved iteratively. This minimal-prefix encoding is described
in :doc:`../compiler/codebuffer`.
