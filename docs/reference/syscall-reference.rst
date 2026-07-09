System-call reference
=====================

System calls are made with the ``SVC`` ``OPR`` sub-operation: the A register
holds the call number and the arguments are read from the stack.  This page is
a quick reference for the three calls; the calling model is described in prose
in :doc:`../architecture/syscalls`, and the numbers and conventions here are
those implemented in ``src/hexsim.hpp``.

Calling convention
------------------

The stack pointer is held in memory word 1, so ``sp = mem[1]``.  Arguments are
passed in the words just above the stack pointer.  Writing ``sp`` for that word
index, the arguments to a call occupy ``mem[sp+1]``, ``mem[sp+2]`` and
``mem[sp+3]`` as needed.  The call number itself is loaded into A before the
``SVC`` executes.

.. list-table:: System calls
   :header-rows: 1
   :widths: 12 8 80

   * - Call
     - A
     - Arguments and effect
   * - ``EXIT``
     - ``0``
     - Halt the processor.  The exit code is taken from ``mem[sp+2]``.  In a
       network, the first processor to call ``EXIT`` sets the system exit code.
   * - ``WRITE``
     - ``1``
     - Write one byte to an output stream.  The byte is ``mem[sp+2]`` and the
       target stream is ``mem[sp+3]``.
   * - ``READ``
     - ``2``
     - Read one byte from an input stream.  The source stream is ``mem[sp+2]``,
       and the byte read is stored back into ``mem[sp+1]``.

Stream semantics
----------------

The stream argument selects which input or output channel the byte is routed
to.  The simulator's I/O layer maps these onto the host's standard input and
output, so ``WRITE`` to the default stream appears on the simulator's
``stdout`` and ``READ`` blocks on the simulator's ``stdin``.  Values read in
are masked to a byte before being stored.

.. seealso::

   :doc:`../architecture/syscalls` for the full description of the system-call
   model, and :doc:`instruction-quick-ref` for the ``SVC`` encoding.
