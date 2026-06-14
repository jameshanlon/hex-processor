The multi-core network
======================

To run X programs with ``par`` and channels on the RTL, ``rtl/network_top.sv``
assembles ``NUM_CORES`` cores around a pair of routers. Each core (see
:doc:`core` and :doc:`memory-and-links`) injects messages into, and is delivered
messages from, a central crossbar. The design follows the transputer
T9000 + C104 model at small scale: a routing switch carrying buffered,
addressed packets, with every message acknowledged end-to-end so that channel
communication stays synchronous. There is deliberately no virtual-channel
multiplexing.

The router
----------

``rtl/router.sv`` is a stateless ``N``×``N`` address forwarder, parameterised by
flit width (``FLIT_W``) and instantiated twice — once per network. Each of the
``NUM_CORES`` input ports has a one-deep registered input buffer; each output
port runs a per-output round-robin arbiter over the input buffers whose
``dst_core`` names that output:

.. literalinclude:: ../../rtl/router.sv
   :language: verilog
   :lines: 1-15

There is no routing table inside the router: a flit's ``dst_core`` field
*directly* selects the output port. The arbiter (``rr[j]`` is the round-robin
pointer for output ``j``) grants one flit per output per cycle and registers it
to that output, freeing the winning input buffer. A stalled output
(``i_out_ready`` low) holds its flit and back-pressures the input, so flits are
never lost or overwritten; one hop costs one cycle, which keeps timing clean and
avoids combinational loops through the crossbar.

The network top
---------------

``rtl/network_top.sv`` wires core ``k`` to router input port ``k`` and router
output port ``k``, for both a **DATA** network and an **ACK** network:

.. literalinclude:: ../../rtl/network_top.sv
   :language: verilog
   :lines: 81-103

Splitting DATA and ACK into two independent routers means an acknowledgement can
never be stuck behind data — the classic request/response deadlock — so the
transport itself is deadlock-free; the only deadlock that can remain is a genuine
occam-style cycle of processes all blocked on channel ops.

Configuration vs. wiring
------------------------

The physical wiring is *static*: ``NUM_CORES`` cores, two ``N``×``N`` crossbars,
fixed port assignments. The *topology* a particular program needs — which core's
slot talks to which other core's slot — is realised entirely by
**configuration**, not by re-elaborating the RTL. At reset the testbench reads
the network container's edges and programs each core's route table over the
``i_cfg_*`` port (``network_top`` decodes ``i_cfg_core`` to a per-core write
enable). This is the hardware counterpart of the compiler's container wiring
described in :doc:`../compiler/networks`: the compiler emits ``(procA, slotA,
procB, slotB)`` edges, and those same edges become the route-table entries that
tell each LIU where its slots lead. One elaboration runs any topology that fits
in ``NUM_CORES``.

Flits
-----

Messages are single-flit, one word per message. The flit structs are defined in
``rtl/hex_pkg.sv``:

.. literalinclude:: ../../rtl/hex_pkg.sv
   :language: verilog
   :lines: 65-81

A **DATA** flit carries ``dst_core`` (selects the router output), ``dst_slot``
(which receive buffer at the destination), ``src_core`` (so the reader can
address the ACK back), and the 32-bit ``word``. An **ACK** flit is just a
``dst_core`` — the original ``src_core`` — because a writer has at most one
outstanding ``OUT``, so an ACK arriving at a core unambiguously completes it. The
testbench mirrors this packing in ``tests/rtl/flit_layout.hpp``: the DATA flit is
a 38-bit value laid out, MSB to LSB, as ``[37:36] dst_core``, ``[35:34]
dst_slot``, ``[33:32] src_core``, ``[31:0] word``, and the ACK flit is the 2-bit
``dst_core``.

Putting it together, sending ``c ! x`` from a writer to a reader's ``c ? v`` is:
the writer's LIU injects a DATA flit toward the reader and stalls; the DATA
router delivers it into the reader's slot buffer; the reader's ``IN`` consumes
the word and injects an ACK flit toward the writer; the ACK router delivers it
and the writer unblocks. Both cores have then synchronised on the communication —
exactly the rendezvous of :doc:`../architecture/channels`.

.. todo:: Add a network topology diagram.
