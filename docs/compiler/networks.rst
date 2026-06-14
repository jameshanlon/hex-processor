Network containers
==================

A single X program compiles to one processor image. When the program describes
a *network* of communicating processes — its ``main`` is a top-level ``par`` —
the compiler instead emits a **network container**: one image per processor,
plus a description of how the processors' channel link slots are wired together.
This page documents how the container is built and what it contains. The
language-level model is described in :doc:`../language/concurrency` and the
hardware channels in :doc:`../architecture/channels`.

When a container is produced
----------------------------

In ``xcmp::Driver::run``, the binary stage first checks whether ``main`` is a
top-level ``par`` (``network::getTopLevelPar``). If it is not, the ordinary
single-image pipeline runs and a plain ``.bin`` is written. If it is, the driver
analyses the network and emits a container instead. The parser enforces that
each branch of a top-level ``par`` is a procedure call — the entry process for
one processor.

Analysing the network
---------------------

``network::analyseNetwork`` (in ``src/xcmp.hpp``) turns the ``par`` into a
``Network`` of processors and wiring edges:

* Each ``par`` branch becomes one processor, running the named entry procedure.
  Its channel arguments are assigned **link slots** in argument order (a
  processor may use at most four channels — the 4-link limit is checked).
* The body of each entry procedure is scanned (``ChannelDirections``) to see
  which channel formals it writes (``!``) and which it reads (``?``).
* Channels are matched by the variable passed as the argument: every channel
  must connect **exactly two** processes, with **exactly one writer and one
  reader**. A violation raises a ``NetworkError``. Each validated channel
  becomes an ``Edge`` recording the two ``(processor, slot)`` endpoints.

Per-processor images
--------------------

For each processor, ``compileProcessorImage`` re-parses the source and rewrites
``main`` to a single call to that processor's entry procedure, passing its
link-slot indices ``0 .. n-1`` as constants, then runs the full single-image
pipeline. Each processor therefore boots straight into its own process, and a
channel formal is simply the integer index of the link slot it uses (a channel
formal is passed by value, like a ``val``).

The container format
--------------------

The container is written by ``emitNetworkContainer`` and read back by
``hexcontainer::read`` in ``src/hexcontainer.hpp``. The reader is shared by the
C++ simulator ``hexsim`` and the Verilator testbench ``hextb`` so the two cannot
drift. The layout is little-endian:

.. literalinclude:: ../../src/hexcontainer.hpp
   :language: cpp
   :lines: 16-25

That is: the magic word ``0x4E584548`` (the ASCII ``"HEXN"``, defined as
``network::CONTAINER_MAGIC`` in the compiler and ``hexcontainer::MAGIC`` in the
reader), the processor and edge counts, the edges (each four words: ``procA``,
``slotA``, ``procB``, ``slotB``), then each processor's image as a size-prefixed
standard single-image binary.

Booting and running a network
-----------------------------

Detection is by the magic word: ``hexcontainer::read`` treats a file that lacks
``"HEXN"`` as a single plain image (the whole file), and one that has it as a
network. ``hexsim`` and ``hextb`` use this to decide whether to boot a single
core or the whole network, wiring up the cores' link slots according to the
edges. ``hexsim`` reports the exit code of the first processor to halt, and
detects deadlock when every core is simultaneously blocked on a channel. The
on-disk single-image and container formats are catalogued in
:doc:`../tools/formats`.
