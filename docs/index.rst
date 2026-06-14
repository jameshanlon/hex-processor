=============
Hex Processor
=============

These documents describe a complete, deliberately minimal computing stack built
from first principles — from logic gates up to a self-hosting programming
language.  The project exists as a reference for understanding how programming
languages correspond to processor hardware: every design decision at each layer
is explained in terms of the layer below it, and nothing is hidden behind
abstraction that is not itself documented here.

The layered stack
-----------------

The documentation is organised bottom-up, following the dependencies between
layers.  Each layer assumes only the one immediately beneath it:

1. **Hex architecture** — the instruction set, registers, encoding, and
   execution model of the Hex processor.  See :doc:`architecture/overview`.
2. **X language** — a small, low-level systems language that compiles directly
   to Hex instructions.  See :doc:`language/overview`.
3. **The compiler** — a single-pass compiler for X, itself written in X
   (self-hosting).  See :doc:`compiler/overview`.
4. **The RTL** — a synthesisable Verilog implementation of the Hex processor.
   See :doc:`hardware/overview`.
5. **The toolchain** — assembler, simulator, linker, and container tools.
   See :doc:`tools/index`.

Historical lineage
------------------

The Hex architecture descends from the Transputer family and the "Simple 42"
teaching processor.  The X language stands in the tradition of BCPL (Martin
Richards) and Occam (David May), sharing their philosophy of a small, portable
language that maps cleanly onto hardware.  The compiler's self-hosting
bootstrap follows the same lineage: an initial implementation in a host
language produces a native compiler that can rebuild itself from source.

How to read these docs
----------------------

The documentation is written as a reference, not a tutorial, and is best read
bottom-up, starting with the architecture.  Readers new to the project may
prefer to begin at :doc:`architecture/overview` before moving to the language
and compiler chapters.  Working examples can be found in the ``examples/``
directory of the repository; the repository's ``README`` contains a quickstart
guide for building and running programs.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   architecture/overview
   architecture/registers
   architecture/instruction-encoding
   architecture/instruction-set
   architecture/execution
   architecture/channels
   architecture/syscalls
   architecture/simulator-model
   language/overview
   language/lexical
   language/program-structure
   language/statements
   language/procedures-functions
   language/expressions
   language/concurrency
   language/examples
   language/grammar
   compiler/overview
   compiler/lexical-analyser
   compiler/syntax-analyser
   compiler/translator
   compiler/codebuffer
   compiler/memory-and-calling
   compiler/codegen-idioms
   compiler/bootstrapping
   compiler/networks
   hardware/overview
   hardware/core
   hardware/memory-and-links
   hardware/network
   hardware/testbench
   tools/index
   tools/formats
   tools/building
   tools/testing
   reference/instruction-quick-ref
   reference/syscall-reference
   reference/primary-sources
   reference/further-reading


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
