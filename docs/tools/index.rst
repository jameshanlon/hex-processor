The toolchain
=============

The project ships six command-line tools that together cover the whole flow
from source to silicon: assembling and disassembling Hex binaries, compiling
the X language, and executing the result either on the C++ simulator or on the
Verilog RTL.  All six are built by CMake (see :doc:`building`) and installed
into ``install/bin``.

.. list-table:: The six tools
   :header-rows: 1
   :widths: 12 88

   * - Tool
     - Purpose
   * - ``hexasm``
     - Assembler: translates ``.S`` Hex assembly into a ``.bin`` image.
   * - ``hexdis``
     - Disassembler: turns a ``.bin`` image back into a readable listing.
   * - ``xcmp``
     - X compiler: compiles a ``.x`` program to a ``.bin`` image (or a network
       container when ``main`` is a top-level ``par``).
   * - ``hexsim``
     - Simulator: executes a single image or a multi-core network container in
       software.
   * - ``xrun``
     - Runner: compiles an X program and immediately executes it on the
       simulator.
   * - ``hextb``
     - Verilator testbench: executes an image or container on the RTL
       multi-core network (requires Verilator).

The image and container file formats produced and consumed by these tools are
documented in :doc:`formats`.

hexasm — the assembler
----------------------

``hexasm`` assembles a single ``.S`` source file into a ``.bin`` image.

.. code-block:: text

   Usage: hexasm file [options]

     file              A source file to assemble
     -h, --help        Display this message
     --tokens          Tokenise the input only
     --instrs          Display the instruction sequence only
     -o, --output file Specify a file for binary output (default a.out)

The default output filename is ``a.out``; pass ``-o`` to choose another.  The
``--tokens`` and ``--instrs`` flags stop after lexing or after instruction
selection respectively, which is useful for inspecting the assembler's
intermediate stages.

Assemble and run the ``hello`` assembly program:

.. code-block:: bash

   $ hexasm tests/asm/hello.S -o hello.bin
   $ hexsim hello.bin
   hello

hexdis — the disassembler
-------------------------

``hexdis`` reads a ``.bin`` image and prints an instruction listing.

.. code-block:: text

   Usage: hexdis file [options]

     file              A binary file to disassemble
     -h, --help        Display this message
     --no-labels       Don't display debug labels

If the image carries a debug-info block (images produced by ``xcmp`` do), the
disassembler prints function labels and resolves addresses to ``symbol+offset``
form.  Each line shows the byte address, the raw instruction byte, the decoded
mnemonic and its operand:

.. code-block:: bash

   $ hexdis hello.bin | head -10
     0x0000  97  BR   7
     0x0001  00  LDAM 0
     ...
     0x0008  51  LDAP 1
     0x0009  94  BR   4
     0x000a  11  LDBM 1
     0x000b  30  LDAC 0
     0x000c  82  STAI 2
     0x000d  d3  SVC

   main:

Pass ``--no-labels`` to suppress the debug labels and print a flat listing:

.. code-block:: bash

   $ hexdis hello.bin --no-labels | head -3

xcmp — the X compiler
---------------------

``xcmp`` compiles an X program to a ``.bin`` image.  When the program's ``main``
ends in a top-level ``par`` block, the output is a *network container* holding
one image per core (see :doc:`formats` and :doc:`../compiler/networks`).

.. code-block:: text

   Usage: xcmp file [options]

     file              A source file to compile
     -h, --help        Display this message
     --tokens          Tokenise the input only
     --tree            Display the syntax tree only
     --tree-opt        Display the optimised syntax tree only
     --insts           Display the intermediate instructions only
     --insts-lowered   Display the lowered instructions only
     --insts-optimised Display the lowered optimised instructions only
     --insts-asm       Display the assembled instructions only
     --memory-info     Report memory information
     -S                Emit the assembly program
     -o, --output file Specify a file for output (default a.out)

The intermediate-stage flags expose every phase of the compiler in turn —
tokens, syntax tree, optimised tree, intermediate and lowered instructions, and
the final assembly — mirroring the pipeline described in
:doc:`../compiler/overview`.  ``-S`` emits a ``.S`` assembly file that
``hexasm`` could then assemble.

Compile and run the ``hello`` X program:

.. code-block:: bash

   $ xcmp examples/hello_putval.x -o hello.bin
   $ hexsim hello.bin
   hello world

hexsim — the simulator
----------------------

``hexsim`` executes a single image or a network container.  It detects the
container magic automatically and boots either one processor or the whole
network.

.. code-block:: text

   Usage: hexsim file [options]

     file              A binary file to simulate
     -h, --help        Display this message
     -d, --dump        Dump the binary file contents
     -t, --trace       Enable instruction tracing
     --max-cycles N    Limit the number of simulation cycles (default: 0)

A ``--max-cycles`` value of ``0`` (the default) means run without a cycle
limit.  For a network container, ``hexsim`` reports the exit code of the *first
processor to halt* and detects deadlock when every core is simultaneously
blocked on a channel.

Run with instruction tracing (``-t``) to see each cycle's execution:

.. code-block:: bash

   $ hexsim hello.bin -t | head -5
   0      0                   BR   7  pc = pc + oreg (7) (0x000008)
   1      8                   LDAP 1  areg = pc (9) + oreg (1) 10
   2      9                   BR   4  pc = pc + oreg (4) (0x00000e)
   3      14     main+0       LDBM 1  breg = mem[oreg (0x000001)] (65536)
   4      15     main+1       STAI 0  mem[breg (65536) + oreg (0) = 0x010000] = areg (10)

The trace columns are, left to right:

.. list-table:: Trace columns
   :header-rows: 1
   :widths: 18 82

   * - Column
     - Meaning
   * - Cycle
     - The simulation cycle number (one instruction per cycle).
   * - PC address
     - The byte address of the instruction being executed.
   * - Symbol+offset
     - The nearest debug symbol and the byte offset past it (blank when no
       debug info is present, as in the bootstrap entry sequence).
   * - Instruction
     - The decoded mnemonic and operand.
   * - Operation
     - The concrete effect on registers or memory, with resolved values.

xrun — compile and run
----------------------

``xrun`` is a convenience front-end that compiles an X program and immediately
executes it on the simulator, without writing a persistent binary.

.. code-block:: text

   Usage: xrun file [options]

     file              A source file to run
     -h, --help        Display this message
     -t, --trace       Enable instruction tracing
     --max-cycles N    Limit the number of simulation cycles (default: 0)

.. code-block:: bash

   $ xrun examples/hello_putval.x
   hello world

Internally ``xrun`` compiles to a temporary ``a.bin`` and then runs it through
the same simulator engine as ``hexsim``, so the ``-t`` and ``--max-cycles``
options behave identically.

hextb — the RTL testbench
-------------------------

``hextb`` runs an image or a network container on the Verilated multi-core RTL
network rather than the C++ model.  It requires the project to have been built
with Verilator (see :doc:`building`); the RTL design is described in
:doc:`../hardware/network`.

.. code-block:: text

   Usage: hextb file [options]

     file              A binary or network container to execute
     -h, --help        Display this message
     -t, --trace       Enable per-core PC tracing
     --max-cycles N    Limit simulation cycles (default: 0)

``hextb`` loads each image into a core's memory, programs the routers from the
container's edge list, fills any unused core with a quiescent halt loop, and
then clocks the network until every active core has exited.  Like ``hexsim``,
it returns the exit code of the first core to call the exit syscall and raises a
deadlock error if no core makes progress for a sustained number of cycles.  Its
``-t`` flag prints per-core program counters each cycle rather than a decoded
instruction trace.

.. seealso::

   :doc:`formats` for the binary and container layouts these tools read and
   write, and :doc:`building` for how to compile the tools themselves.
