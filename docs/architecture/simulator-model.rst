The reference simulator
========================

The clearest way to understand the Hex architecture is to read a complete
interpreter for it. The whole machine — fetch, decode, execute — fits in a short
loop over an array of memory and four register variables. This page presents
that loop as a pedagogical model and then relates it to the production C++
simulator, ``hexsim``.

State
-----

The model needs only the memory array and the registers:

.. code-block:: c

   uint32_t mem[MEM_SIZE];   /* word-addressed memory          */
   uint32_t pc;              /* program counter (byte address) */
   uint32_t areg, breg;      /* the two evaluation registers   */
   uint32_t oreg;            /* the operand register           */

The fetch–decode–execute loop
-----------------------------

Each iteration fetches the instruction byte at ``pc``, advances ``pc``, folds the
low nibble into ``oreg``, and switches on the high nibble. Note how most cases
end by clearing ``oreg`` to zero, while ``PFIX``/``NFIX`` instead shift it:

.. code-block:: c

   for (;;) {
     /* Fetch one byte from the word-addressed memory, then advance pc. */
     uint8_t inst = (mem[pc >> 2] >> ((pc & 3) << 3)) & 0xFF;
     pc = pc + 1;
     oreg = oreg | (inst & 0xF);     /* accumulate the operand nibble */

     switch (inst >> 4) {            /* decode on the high nibble     */
     case LDAM: areg = mem[oreg];          oreg = 0; break;
     case LDBM: breg = mem[oreg];          oreg = 0; break;
     case STAM: mem[oreg] = areg;          oreg = 0; break;
     case LDAC: areg = oreg;               oreg = 0; break;
     case LDBC: breg = oreg;               oreg = 0; break;
     case LDAP: areg = pc + oreg;          oreg = 0; break;
     case LDAI: areg = mem[areg + oreg];   oreg = 0; break;
     case LDBI: breg = mem[breg + oreg];   oreg = 0; break;
     case STAI: mem[breg + oreg] = areg;   oreg = 0; break;
     case BR:   pc = pc + oreg;            oreg = 0; break;
     case BRZ:  if (areg == 0)      pc = pc + oreg; oreg = 0; break;
     case BRN:  if ((int)areg < 0)  pc = pc + oreg; oreg = 0; break;
     case PFIX: oreg = oreg << 4;                   break;
     case NFIX: oreg = 0xFFFFFF00 | (oreg << 4);    break;
     case OPR:
       switch (oreg) {
       case BRB: pc = breg;          oreg = 0; break;
       case ADD: areg = areg + breg; oreg = 0; break;
       case SUB: areg = areg - breg; oreg = 0; break;
       case SVC: syscall();          oreg = 0; break;
       }
       break;
     }
   }

A few points are worth drawing out:

* **Fetch.** ``mem[pc >> 2]`` selects the word and ``(pc & 3) << 3`` is the bit
  offset of the byte within it, so the byte extraction reflects the word-addressed
  memory with byte-addressed instructions described in :doc:`overview`.
* **Operand accumulation.** Every instruction folds its nibble into ``oreg``
  before doing anything else. ``PFIX`` and ``NFIX`` leave ``oreg`` shifted rather
  than cleared, so the next instruction's nibble extends it — this is the prefix
  mechanism of :doc:`instruction-encoding`.
* **``OPR``** does not use ``oreg`` as a value but as a sub-op selector, branching
  to add, subtract, branch-to-``breg``, or a supervisor call.

Helper functions
----------------

The loop above defers a few details to small helpers:

* **A load helper** reads the program image into ``mem`` at reset, including the
  word-0 branch to the entry point and the word-1 stack pointer.
* **``syscall``** reads the stack pointer from ``mem[1]`` and dispatches on
  ``areg``: exit, write a byte, or read a byte (see :doc:`syscalls`).
* **``simout`` / ``simin``** back the write and read calls, sending a byte to a
  stream or fetching one from it.

The channel operations ``IN``/``OUT`` are omitted from this minimal loop because
they require more than one processor; they are covered in :doc:`channels`.

Relationship to ``hexsim``
--------------------------

The production simulator ``hexsim`` is this same loop, made into a ``Processor``
class with its registers and ``memory`` array as members and the ``switch`` as a
``step()`` method. On top of the core model it adds instruction **tracing**,
**multi-core networks** of processors connected by channels, and **deadlock
detection** when every processor is blocked on a channel. See
:doc:`../tools/index` for how to drive it.
