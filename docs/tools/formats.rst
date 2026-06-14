Binary and container formats
============================

The toolchain uses two on-disk formats: a single-image ``.bin`` file holding
one processor's program, and a *network container* holding one image per core
plus the channel wiring between them.  Both are little-endian and word-oriented
(a word is 32 bits).  The single-image format is defined by
``src/heximage.hpp`` and the container by ``src/hexcontainer.hpp``; the two
share the same serialisation primitives so the writers (``hexasm``, ``xcmp``)
and the readers (``hexsim``, ``hexdis``, ``hextb``) cannot drift.

The single-image format
-----------------------

An image is a program-size word, the program itself, and an optional debug-info
block.  There is no separate header beyond the size word: the first instruction
of the program *is* the entry point, and the compiler arranges for it to be a
branch (``BR``) over the data area to ``main``.

.. list-table:: Image layout
   :header-rows: 1
   :widths: 22 14 64

   * - Field
     - Size
     - Description
   * - ``programSizeWords``
     - 1 word
     - The number of 32-bit words of program that follow (code, constants and
       string data).  This is the value ``hexdis`` and the simulators use to
       know where the program ends and the optional debug block begins.
   * - program
     - ``programSizeWords`` words
     - The instruction bytes, packed four to a word, followed by any constant
       and string data the program references.  Execution begins at the first
       byte; the leading instruction branches over the data to ``main``.
   * - debug info
     - variable, optional
     - A string table and symbol table (see below).  Present in images emitted
       by ``xcmp``; absent from hand-written assembly that defines no symbols.

The debug-info block, when present, is laid out as:

.. list-table:: Debug-info block
   :header-rows: 1
   :widths: 22 16 62

   * - Field
     - Size
     - Description
   * - ``numStrings``
     - 1 word
     - The number of names in the string table.
   * - strings
     - variable
     - ``numStrings`` null-terminated strings, one per symbol (no string
       pooling).
   * - ``numSymbols``
     - 1 word
     - The number of symbols in the symbol table.
   * - symbols
     - ``numSymbols`` × 2 words
     - Each symbol is a ``(stringIndex, byteOffset)`` pair: an index into the
       string table giving the symbol's name, and the byte offset in the
       program that it labels.

``hexdis`` reads this block to print function labels and ``symbol+offset``
addresses; ``hexsim`` reads it to annotate its instruction trace.  The buffer
that accumulates the program and its symbols during compilation is described in
:doc:`../compiler/codebuffer`.

The network-container format
----------------------------

A network container packages several images together with the point-to-point
channel wiring that connects their link slots.  It begins with the 32-bit magic
``0x4E584548`` (the ASCII bytes ``"HEXN"``); any file lacking this magic is
treated as a single plain image, which is how ``hexsim`` and ``hextb`` accept
both formats through one code path.

.. list-table:: Container layout
   :header-rows: 1
   :widths: 22 16 62

   * - Field
     - Size
     - Description
   * - ``magic``
     - 1 word
     - ``0x4E584548`` (``"HEXN"``).  Identifies the file as a network
       container.
   * - ``numProcessors``
     - 1 word
     - The number of processor images that follow.
   * - ``numEdges``
     - 1 word
     - The number of channel edges in the wiring table.
   * - edges
     - ``numEdges`` × 4 words
     - Each edge is a ``(procA, slotA, procB, slotB)`` tuple: a bidirectional
       channel connecting link slot ``slotA`` of processor ``procA`` to link
       slot ``slotB`` of processor ``procB``.
   * - images
     - variable
     - ``numProcessors`` size-prefixed images.  Each is a ``uint32``
       ``imageSizeBytes`` followed by exactly that many bytes of a standard
       single-image binary (size word + program + optional debug info).

When loading a container, ``hexsim`` instantiates one processor per image and
builds the channels from the edge list, while ``hextb`` loads each image into a
core's memory and programs the RTL routers from the same edges.  The compiler
side of this — how a top-level ``par`` becomes a container, and how slots are
assigned — is described in :doc:`../compiler/networks`.

.. seealso::

   :doc:`../compiler/codebuffer` for how images are assembled in memory, and
   :doc:`../compiler/networks` for how the container's edges and images are
   generated.
