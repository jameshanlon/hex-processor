Primary sources
===============

The Hex architecture and the X language originate in a set of notes by David
May, included here in their original form.  These documents are the authoritative
description of the *original* design and a valuable narrative companion to the
present reference.  They predate the additions made in this project — channels,
``par`` and the network container, and the current C++ toolchain — so where the
two disagree, these reference pages describe the project as it stands today and
the PDFs describe the design they grew from.

The original notes
------------------

:download:`The Hex Architecture (hexb.pdf) <../PDFs/hexb.pdf>`
   David May, 2014.  Describes the processor itself: the four registers, the
   8-bit instruction format with its 4-bit opcode and 4-bit operand, the
   ``PFIX``/``NFIX`` mechanism for building larger operands, the full
   instruction set, and a small C reference simulator.

:download:`X and Hex (xhexnotes.pdf) <../PDFs/xhexnotes.pdf>`
   David May, 2014.  The X language reference together with the Hex ISA and an
   overview of the compiler internals — the lexer, syntax analyser, translator
   and code buffer pipeline — along with the memory layout and calling
   convention used by the generated code.

:download:`The X compiler in X (xhexb.pdf) <../PDFs/xhexb.pdf>`
   The source listing of the X compiler written in X itself: the self-hosting
   bootstrap from which the compiler can rebuild itself.

:download:`The X compiler in Hex assembly (xhexba.pdf) <../PDFs/xhexba.pdf>`
   The same compiler compiled down to Hex assembly: the bootstrap output
   listing, i.e. the native form that runs the self-hosting build.

.. seealso::

   :doc:`../compiler/bootstrapping` for how the self-hosting compiler is built,
   and :doc:`further-reading` for the wider historical context.
