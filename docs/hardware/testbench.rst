Testbench
=========

The RTL is exercised at two levels: ``hextb``, a Verilator front-end that runs a
whole image or network container on the multi-core network, and a set of small
C++ unit testbenches under ``tests/rtl/`` that check individual modules.

Verilator
---------

All of the RTL testing is driven through `Verilator
<https://www.veripool.org/verilator/>`_, which compiles the SystemVerilog into a
cycle-accurate C++ model. The project targets a recent Verilator (the 5.x
series); ``≥ ~4.200`` is the practical floor, and distribution packages older
than that (for example the 4.038 shipped by some apt repositories) are too old.
The build uses a suitable system Verilator if one is found and otherwise fetches
and builds a pinned version automatically — see :doc:`../tools/building` for the
toolchain setup and :doc:`../tools/testing` for running the suite.

The hextb tool
--------------

``tools/hextb.cpp`` is the Verilator testbench for the full system. It elaborates
``network_top`` (as ``Vntb``) and runs either a single image or a multi-core
network container on it. Its flow is:

#. **Load.** Read the container, fill every core's memory with a quiescent halt
   loop, then copy each image's code into its core's ``memory_q`` (reached through
   the generate-loop instance names, e.g. ``g_core[k].u_core``). A plain binary is
   one image; a ``par`` program is a container of several.
#. **Configure routes.** Hold reset for a few cycles, then for each container edge
   program both endpoints' route tables via the ``i_cfg_*`` port — the same
   ``(procA, slotA, procB, slotB)`` edges the compiler emitted (see
   :doc:`../compiler/networks`) — then release reset.
#. **Run.** Clock the design, and each cycle service any core asserting
   ``o_syscall_valid``. Syscalls (``EXIT`` / ``WRITE`` / ``READ``) read their
   arguments from that core's own memory through the shared ``HexSimIO``; the
   first ``EXIT`` sets the system exit code.
#. **Detect deadlock.** A channel rendezvous legitimately freezes the
   participating cores' PCs for a few cycles. The harness only reports deadlock
   after a sustained stretch (``DEADLOCK_THRESHOLD`` cycles) with no PC change and
   no syscall on any core; ``--max-cycles`` is a runaway backstop.

Because the same container runs unchanged on both ``hexsim`` and ``hextb``, and
``hexsim`` is a semantically simple functional model of the channel rendezvous,
``hexsim`` serves as an independent golden model: the two must produce identical
program output and exit code even though their channel *timing* differs.

Command-line usage::

  hextb <file> [-t|--trace] [--max-cycles N]

where ``file`` is a binary or network container, ``-t`` enables per-core PC
tracing, and ``--max-cycles`` bounds the simulation (``0`` = unbounded). Passing
``+trace`` to the underlying Verilated model dumps a VCD waveform.

RTL unit testbenches
--------------------

``tests/rtl/`` holds focused, self-checking testbenches — each instantiates one
Verilated module, drives it directly, and ``assert``\ s on the results:

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - File
     - What it exercises
   * - ``core_tb.cpp``
     - A whole ``core`` (``Vcore``): loads a tiny program that does ``OUT`` on a
       configured slot, checks the injected DATA flit's fields, verifies the
       processor's PC stays frozen until the ACK arrives, then confirms it
       unblocks once the ACK is returned.
   * - ``liu_tb.cpp``
     - The ``link_interface`` in isolation (``Vlink_interface``): an ``OUT`` that
       injects the right DATA flit and stalls until an ACK; an ``IN`` that blocks
       until its slot buffer fills then delivers the word and emits an ACK; and
       per-slot independence (a delivery to one slot must not satisfy an ``IN``
       waiting on another).
   * - ``router_tb.cpp``
     - The ``router`` crossbar (``Vrouter``): a flit is forwarded to the output
       named by ``dst_core`` and is held under output back-pressure (not lost),
       and round-robin arbitration is fair when two inputs target one output (no
       starvation).
   * - ``flit_layout.hpp``
     - Shared helper header — not a test. It defines the DATA/ACK flit
       bit-packing (``make_dnet_flit`` and the ``flit_*`` accessors) in one place
       so ``core_tb`` and ``liu_tb`` agree on the layout described in
       :doc:`network`.

Together these check the messaging engine bottom-up: the router forwards and
arbitrates correctly, the link interface implements the OUT/IN/ACK protocol with
independent per-slot buffers, and a full core ties the processor's stall path to
that protocol — while ``hextb`` validates the assembled network end-to-end
against the ``hexsim`` golden model.
