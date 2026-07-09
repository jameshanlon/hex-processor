Concurrency
===========

X expresses concurrency in the style of the Transputer and occam, kept as simple
as possible: **one process maps to one physical processor**. There is no
scheduler and no time-slicing — parallelism comes from *placing* sequential
processes on real Hex cores and wiring them together with channels. Parallel
processes share no memory; they communicate only by passing messages.

This page covers the language constructs (``chan``, ``par`` and the ``!`` / ``?``
operators). The underlying machine mechanism is described in
:doc:`../architecture/channels`, and the multi-core hardware in
:doc:`../hardware/network`.

Channels
--------

A *channel* is a point-to-point link that carries one word per message. A
channel is declared with ``chan`` (at global scope, or as a local declaration of
a procedure)::

   chan a;
   chan b;

and passed into a process as a ``chan`` formal::

   proc relay(chan in, chan out) is ...

A channel connects exactly two processes: one that sends on it and one that
receives. There is no shared memory between parallel processes, so channels are
the *only* way they exchange data.

Send and receive
-----------------

Two statements operate on channels:

* **Send**, ``channel ! expression``, evaluates the expression and sends the
  word over the channel.
* **Receive**, ``channel ? element``, receives a word and stores it in the
  variable or array element.

Communication is a synchronous, unbuffered **rendezvous**: the first party to
reach the channel blocks until its partner is ready, the word is exchanged, and
both continue. So a relay that forwards one value is simply::

   proc relay(chan in, chan out) is
     var v;
     { in ? v; out ! v }

``par``
-------

A ``par`` block runs its branches **in parallel, each on a separate processor**,
connected by the channels passed to them::

   par { source(a); relay(a, b); sink(b) }

Each branch must be a procedure call — it is the entry process for one
processor. The branches are written inside braces and separated by semicolons,
like a sequence, but they execute concurrently rather than in order. The
channels named in the calls (here ``a`` and ``b``) are what wire the processors
together: ``source`` and ``relay`` share channel ``a``, ``relay`` and ``sink``
share ``b``. The same procedure may be placed on more than one processor with
different channel arguments — ``ring.x`` reuses one ``forwarder`` on two cores.

``par`` is static and has no join: it is the program's top-level structure, each
branch runs to its own termination, and the machine halts when all branches
have halted. There is no code "after" a ``par``.

When ``main`` ends in a top-level ``par``, the compiler does not emit a single
processor image. Instead it emits a **network container**: one image per core,
plus a table describing how the cores' channel slots are wired together. See
:doc:`../compiler/networks` for how this is built, and
:doc:`../architecture/channels` for the channel mechanism.

A complete example
------------------

The three-stage pipeline ``source -> relay -> sink`` runs on three processors
connected by two channels. The source emits a character, the relay forwards it,
and the sink prints it:

.. literalinclude:: ../../examples/pipe.x
   :language: text

Other concurrent example programs — ``pingpong.x`` (two cores exchanging a
value), ``ring.x`` (a token ring), ``buffer.x`` (streaming with a sentinel),
``sieve.x`` (a concurrent prime sieve) and more — are surveyed on the
:doc:`examples` page.
