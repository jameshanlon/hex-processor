Memory and links
================

A core couples the processor to two stateful units: the private ``memory`` it
fetches from and stores to, and the ``link_interface`` that turns the
processor's ``IN`` / ``OUT`` hand-off into messages on the network. Both are
instantiated inside ``rtl/core.sv`` alongside the processor.

Memory
------

``rtl/memory.sv`` is a single-port-per-function model: a read-only instruction
fetch port and a read/write data port over one register array.

.. literalinclude:: ../../rtl/memory.sv
   :language: verilog
   :lines: 1-15

The storage is word-addressed — ``memory_q`` is an array of ``MEM_WIDTH`` (32)
bit words, ``MEM_DEPTH`` deep — and writes are word writes: on a clocked
``i_d_valid && i_d_we`` the 32-bit ``i_d_data`` is stored at the word address
``i_d_addr``. The data read port is purely combinational
(``o_d_data = memory_q[i_d_addr]``), so a load returns its word in the same cycle
the processor presents the address.

Instructions, however, are bytes (see
:doc:`../architecture/instruction-encoding`), so the fetch port extracts one byte
from the addressed word. The fetch address ``i_f_addr`` is a *byte* address; its
top bits select the word and its low two bits select the byte within that word:

.. literalinclude:: ../../rtl/memory.sv
   :language: verilog
   :lines: 24-28

``fetch_byte_addr`` turns the byte offset ``i_f_addr[1:0]`` into a bit offset
(``<< 3``, i.e. ×8) and the indexed part-select ``[fetch_byte_addr +: 8]`` reads
the corresponding 8-bit instruction out of the 32-bit word. This is why the
processor's program counter is a byte address while its data addresses are word
addresses.

The link interface
------------------

``rtl/link_interface.sv`` (the "LIU") is the per-core messaging engine. It
realises the synchronous channel rendezvous of :doc:`../architecture/channels` in
hardware: the first party to a channel blocks until the other arrives, the word
is exchanged exactly once, and both then continue. Its port list shows the three
faces it presents — the processor side, the reset-time route-table config, and
the DATA and ACK network ports:

.. literalinclude:: ../../rtl/link_interface.sv
   :language: verilog
   :lines: 1-34

Route table and receive buffers
-------------------------------

Each core's logical channel slots (``0`` … ``NUM_LINKS-1``) are *addresses*, not
wires. The LIU holds a small **route table** mapping each slot to a
``(dst_core, dst_slot)`` pair; it is written at reset over the ``i_cfg_*`` port
from the network container's edges (so the same binary runs unchanged on
``hexsim`` and on the RTL). It also holds **per-slot receive buffers** — one
one-word register per slot (``rx_valid`` / ``rx_src`` / ``rx_word``). Because
each ``(core, slot)`` channel has a single writer with at most one outstanding
message, one buffer per slot can never overflow, and indexing by ``dst_slot``
stops a message parked for one channel from blocking another.

The rendezvous handshake
------------------------

The processor drives ``i_op_out`` / ``i_op_in`` with the slot in ``i_slot`` and
the word in ``i_areg``, and watches ``o_busy`` (which becomes the processor's
``stall``), ``o_done``, and ``o_in_word``. Internally a four-state FSM
(``IDLE``, ``OUT_SEND``, ``OUT_WAIT``, ``IN_ACK``) runs the protocol against the
two networks:

* **OUT** — from ``IDLE`` the FSM enters ``OUT_SEND`` and asserts ``o_dnet_valid``
  with a DATA flit ``{dst_core, dst_slot, src_core=i_core_id, word=i_areg}`` taken
  from the route table. When the DATA network accepts it (``i_dnet_in_ready``) it
  moves to ``OUT_WAIT`` and the processor stays stalled until the matching **ACK**
  arrives on ``i_anet_valid``, at which point ``o_done`` pulses and it returns to
  ``IDLE``.
* **IN** — the LIU continuously accepts delivered DATA flits into the addressed
  slot buffer (``o_dnet_out_ready`` is high whenever that slot is empty),
  independent of the FSM. An ``IN`` on a slot blocks in ``IDLE`` until that slot's
  ``rx_valid`` is set; it then enters ``IN_ACK``, presents the buffered word on
  ``o_in_word``, and emits an ACK addressed back to the sender (``rx_src``). When
  the ACK network takes the ACK (``i_anet_in_ready``) it pulses ``o_done``, clears
  the slot, and returns to ``IDLE``.

The decisive rule is **ACK-on-consume**: the writer only unblocks because the
reader executed its ``IN`` and consumed the word — not merely because the word
landed in a buffer. That is exactly the occam/X synchronous channel semantics of
:doc:`../architecture/channels`, preserved despite the buffering in the network.
The split into separate DATA and ACK networks (see :doc:`network`) keeps ACKs
from ever queuing behind DATA, which is what makes the transport deadlock-free.
