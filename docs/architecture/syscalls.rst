System calls
============

A program running on Hex reaches the outside world — input, output, and
termination — through *system calls*. These are not separate instructions but a
single ``OPR`` sub-operation, ``SVC`` (operand ``0x3``), that dispatches on a
small set of call numbers.

The ``SVC`` mechanism
---------------------

Executing ``OPR SVC`` performs a system call. The call number is taken from
``areg``, and any further arguments are read from, and results written back to,
the running program's stack.

The stack pointer lives in word 1 of memory (see :doc:`registers`). On an
``SVC``, the machine reads ``sp = mem[1]`` and then locates the call's arguments
and result relative to ``sp``. The defined calls are:

.. list-table::
   :header-rows: 1
   :widths: 14 8 78

   * - Call
     - ``areg``
     - Effect
   * - ``EXIT``
     - ``0``
     - Halt the processor. The exit code is taken from ``mem[sp + 2]``.
   * - ``WRITE``
     - ``1``
     - Write one byte to a stream. The byte value is ``mem[sp + 2]`` and the
       stream id is ``mem[sp + 3]``.
   * - ``READ``
     - ``2``
     - Read one byte from the stream given by ``mem[sp + 2]`` and store it into
       ``mem[sp + 1]``.

Any other value in ``areg`` is an invalid system call and raises a runtime error.

By convention, byte input is truncated to its low 8 bits when stored, matching
the behaviour of the X compiler and the hardware; the simulator can optionally
sign-extend instead, which is useful when testing with negative values.

Streams
-------

The ``WRITE`` and ``READ`` calls take a stream id that selects where the byte
goes or comes from:

* Stream ids below 256 use the default console streams — standard output for
  ``WRITE`` and standard input for ``READ``. This is the usual case: stream
  ``0`` is the console.
* Stream ids of 256 and above are *file-backed*. The file index is taken from
  bits [10:8] of the id, selecting one of eight files named ``simout``\ *N* for
  output and ``simin``\ *N* for input, opened on first use.

Exit codes
----------

When a processor performs an ``EXIT`` call it halts and reports the exit code
read from its stack. In a multi-processor network the system's exit code is that
of the first processor to exit. The runner and simulator front-ends surface this
exit code to the caller; see :doc:`../tools/index`.

See also
--------

For the full numeric reference of each call's number, arguments, and stack
layout, see :doc:`../reference/syscall-reference`.
