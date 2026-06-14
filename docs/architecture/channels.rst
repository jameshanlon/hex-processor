Channels
========

Hex processors communicate with one another through *channels*: point-to-point
links carrying single-word messages. This is the architecture's only mechanism
for inter-processor communication — there is no shared memory between
processors. The model follows the discipline of the Transputer and occam,
deliberately kept as simple as possible: one process runs on one physical
processor, with no scheduler and no buffering.

The ``IN`` and ``OUT`` operations
---------------------------------

Channel communication uses two ``OPR`` sub-operations,
``IN`` (operand ``0x4``) and ``OUT`` (operand ``0x5``). Both name a channel and a
data word through the registers, symmetrically with how ``ADD`` and ``SUB`` use
``areg`` and ``breg``:

* ``breg`` selects one of the processor's **link slots** — the channel to use.
* ``areg`` carries the data word: it is the value *received* by ``IN`` and the
  value *sent* by ``OUT``.

So ``OPR OUT`` writes the word in ``areg`` to the channel on slot ``breg``, and
``OPR IN`` reads a word from the channel on slot ``breg`` into ``areg``.

Blocking rendezvous
-------------------

A channel transfer is a **synchronous, unbuffered rendezvous**. The first party
to arrive at the channel blocks until its partner is also ready. When both are
present, exactly one word is copied from the writer to the reader and both
processors continue. There is no queue and no buffering: the communication
itself is the point of synchronisation between the two processors.

While a processor is blocked on a channel operation, its program counter is not
advanced — the operation simply has not completed yet. When the partner arrives
and the rendezvous occurs, the blocked processor resumes with the transferred
word in ``areg`` and steps past the instruction. No "blocked" state is left in
the architectural registers.

Link slots and wiring
---------------------

Each processor has a fixed, small number of channel link slots — four. A
processor can therefore be wired to at most four channels at once, which matches
the hardware link budget. The wiring (which slot on one processor connects to
which slot on another) is fixed when a network is built; see
:doc:`../compiler/networks`.

Operating on a slot that is not wired to a channel — a slot index outside the
valid range, or a valid slot with no channel attached — is a runtime error. The
simulator reports it, naming the offending processor and program counter, as a
backstop for cases that cannot be ruled out statically.

If every processor in a network becomes blocked on a channel with no partner
able to proceed, the system is deadlocked. The simulator detects this and
reports it, listing the channel slot each processor is blocked on.

Mapping to the X language and to networks
-----------------------------------------

In the :doc:`X language <../language/overview>`, channels appear as the ``chan``
type and the statement-level operators ``!`` (send) and ``?`` (receive). The
compiler lowers ``c ! e`` to evaluating ``e`` into ``areg``, loading the slot
for ``c`` into ``breg``, and emitting ``OPR OUT``; ``c ? v`` loads the slot into
``breg``, emits ``OPR IN``, and stores ``areg`` into ``v``. The language-level
view of concurrency and message passing is described in
:doc:`../language/concurrency`.

A ``chan`` value is represented at runtime simply as the integer index of a link
slot on the processor running that code, which is why the same procedure can be
reused on several processors with different wirings. When ``main`` is a top-level
``par``, the compiler emits a *network container* of one image per processor plus
the slot wiring — see :doc:`../compiler/networks` for how the network is built
and :doc:`../hardware/network` for how it is realised in hardware.
