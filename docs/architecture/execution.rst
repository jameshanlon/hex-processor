Instruction execution
=====================

The execution cycle
-------------------

Hex executes instructions in a simple three-stage cycle:

#. **Fetch.** Read the instruction byte addressed by ``pc``. Because memory is
   word-addressed but instructions are bytes, the word is selected by the high
   bits of ``pc`` and the byte within it by the low bits.
#. **Increment.** Advance ``pc`` to the next instruction byte.
#. **Execute.** Fold the instruction's low nibble into ``oreg``
   (``oreg = oreg | (inst & 0xf)``), then perform the operation selected by the
   high nibble. Most operations clear ``oreg`` to ``0`` afterwards; the ``PFIX``
   and ``NFIX`` prefixes instead leave a shifted operand for the next cycle.

The branch instructions complete by overwriting ``pc`` rather than letting the
increment stand, and the channel operations ``IN``/``OUT`` may suspend the cycle
mid-execution until their partner is ready (see :doc:`channels`).

The datapath
------------

The same cycle is realised in hardware by a small datapath. Its components are:

* **A multiplexor** — selects the left arithmetic input from ``areg``, ``pc``,
  ``oreg``, or zero, depending on the instruction.
* **B multiplexor** — selects the right arithmetic input from ``breg``,
  ``oreg``, or zero.
* **Arithmetic unit** — adds or subtracts its two inputs to produce an address
  or a result.
* **Memory** — addressed by the arithmetic unit's output; its write data comes
  from ``areg``.
* **Result multiplexor** — selects what is written back to the registers, either
  the memory read data or the arithmetic unit's output.
* **Instruction register, decoder and control matrix** — hold the fetched
  instruction byte and derive the multiplexor selects, the arithmetic operation,
  and the memory and register write enables for the current instruction.
* **Clock and timing generator** — sequences the fetch, increment, and execute
  steps.

.. todo:: Add a datapath block diagram.

Relationship to the implementations
-----------------------------------

This cycle and datapath are mirrored in two places. The C reference
interpreter in :doc:`simulator-model` implements the same fetch-decode-execute
loop in software, and the SystemVerilog core in :doc:`../hardware/core`
realises the datapath above as real hardware. The two are kept behaviourally
identical, so a program produces the same results whether it runs on the
simulator or on the RTL.
