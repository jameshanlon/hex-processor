Code buffer
===========

The code buffer holds the program as an ordered list of directives and is
responsible for turning that representation into an executable binary. In
``src/xcmp.hpp`` the ``xcmp::CodeBuffer`` class accumulates the directives
emitted by code generation (instructions in ``instrs`` and statically
allocated words in ``data``), and provides the ``gen…`` helpers used
throughout translation. The conversion of the final directive list into bytes
is performed by ``hexasm::CodeGen`` in ``src/hexasm.hpp``, which the compiler
invokes once the directive stream is complete.

Directives
----------

Each entry in the program is a ``hexasm::Directive``: a ``Label`` (zero size),
a ``Data`` word (always four bytes, word-aligned), an ``InstrImm`` (an
instruction with an immediate value), an ``InstrLabel`` (an instruction whose
operand is a label, marked relative or absolute), an ``InstrOp`` (a one-byte
``OPR`` instruction) or ``Padding``. A directive knows its own size and its
assigned byte offset. Crucially, the size of an immediate or label-relative
instruction is *not* fixed: it depends on the magnitude of the value it
encodes, because larger values need more PFIX/NFIX prefix nibbles.

Minimal-prefix selection
------------------------

An operand value is emitted as a single 4-bit nibble in the instruction itself,
preceded by as many ``PFIX`` (positive) or ``NFIX`` (negative) prefix
instructions as are needed to supply the remaining nibbles. ``numNibbles``
computes how many nibbles a value needs, and ``InstrImm::getSize`` /
``InstrLabel::getSize`` use it to report the encoded length (negative values
that fit in one nibble still need an ``NFIX``, hence a minimum of two). The
compiler always picks the shortest encoding for the value, so small constants
and nearby branches cost a single byte.

For a *relative* label reference there is a circularity: the offset to the
target depends on the encoded length of the instruction, but that length
depends on the offset. ``instrLen`` resolves it by increasing the length until
it is consistent with the distance it has to span:

.. literalinclude:: ../../src/hexasm.hpp
   :language: cpp
   :lines: 301-307

Iterative offset resolution
---------------------------

Because instruction sizes depend on offsets, and offsets depend on the sizes of
all preceding instructions, the program cannot be laid out in a single pass.
``hexasm::CodeGen::resolveLabels`` repeats the layout until it stabilises: on
each iteration it walks the program assigning byte offsets and label values,
and it stops when a full pass leaves the total size unchanged.

.. literalinclude:: ../../src/hexasm.hpp
   :language: cpp
   :lines: 731-782

Each iteration aligns ``Data`` to a four-byte boundary, records each label's
byte offset, and rewrites each instruction's operand: a relative reference
becomes ``target - here - instrLen`` and an absolute reference becomes the
word-aligned target shifted right by two (an absolute *word* address). As
offsets shrink, some branches need fewer prefix bytes, which shifts later
offsets, so the process re-expands (and contracts) until the load points no
longer move. After resolution the program is padded out to a word boundary.

Output format
-------------

The binary image is produced by ``emitImage`` (used directly for a single image
and via ``emitBin`` for a file). Its layout, shared with the readers through
``src/heximage.hpp``, is:

* a ``uint32`` header giving the program size in **words**;
* the program bytes — ``emitProgramBin`` writes each directive, emitting the
  PFIX/NFIX prefix bytes followed by the opcode nibble for instructions, the
  raw word for data (with alignment padding), and zero bytes for padding;
* an optional debug-info block: a string table followed by a symbol table
  mapping ``func``/``proc`` names to byte offsets.

The very first instruction of every image is a ``BR`` to the program's
``start`` label, and the second word (the ``SP_VALUE`` slot) holds the initial
stack pointer; this fixed preamble is what lets the simulator boot any image by
jumping to word 0. The memory map behind those first words is described in
:doc:`memory-and-calling`, the instruction encoding in
:doc:`../architecture/instruction-encoding`, and the on-disk image and network
container layouts in :doc:`../tools/formats`.
