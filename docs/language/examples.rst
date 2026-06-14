Example programs
================

The ``examples/`` directory holds runnable X programs. They range from one-line
programs up to ``xhexb.x``, a complete X compiler written in X. This page tours
them; see :doc:`../tools/index` for how to compile and run a program (``xcmp``,
``hexsim``, ``xrun``).

Sequential programs
-------------------

These run on a single processor.

.. list-table::
   :header-rows: 1
   :widths: 24 76

   * - Program
     - Description
   * - ``exit.x``
     - The smallest program: ``main`` is ``skip``.
   * - ``echo_char.x``
     - Reads a byte and writes it back out.
   * - ``hello_putval.x``
     - Prints ``hello world`` one character at a time with ``putval``.
   * - ``hello_prints.x``
     - Prints a string literal using a ``prints`` routine that unpacks the
       packed string layout.
   * - ``strlen.x``
     - Computes the length of a packed string.
   * - ``printn.x`` / ``printhex.x``
     - Print a number in decimal / hexadecimal.
   * - ``fib.x``
     - Recursive Fibonacci (the introductory example).
   * - ``fac.x``
     - Recursive factorial.
   * - ``gcd.x``
     - Greatest common divisor by Euclid's algorithm; uses the ``div`` / ``rem``
       helpers.
   * - ``collatz.x``
     - Length of the Collatz (3n+1) sequence; ``while`` loops and ``div`` /
       ``rem``.
   * - ``ackermann.x``
     - The Ackermann function — deep non-primitive recursion; a stress test of
       calls and frames.
   * - ``primes.x``
     - Prints primes below ``n`` by trial division; nested loops and early
       ``return``.
   * - ``binsearch.x``
     - Binary search over a sorted array.
   * - ``reverse.x``
     - Reverses an array in place with a two-index swap loop.
   * - ``bubblesort.x``
     - Bubble sort of an array, with a checker function.
   * - ``hanoi.x``
     - Towers of Hanoi: recursion that drives output.
   * - ``div.x`` / ``mul.x`` / ``mul2.x`` / ``exp2.x``
     - Software division, multiplication and powers of two (X has no built-in
       ``*`` or ``/``).
   * - ``globals.x``
     - Exercises global ``var`` / ``val`` / ``array`` declarations and
       constant-expression array sizes.
   * - ``xhexb.x``
     - A complete X compiler written in X — the largest example program.

Concurrent programs
-------------------

These end in a top-level ``par`` and compile to a *network* of cores (each
program comments which cores and channels it uses). They mirror the table in the
project README.

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - Program
     - What it demonstrates
   * - ``pipe.x``
     - Three-stage pipeline (``source -> relay -> sink``).
   * - ``pingpong.x``
     - Two cores exchanging a value and echoing it back.
   * - ``ring.x``
     - Token ring; reuses one ``forwarder`` process on two cores.
   * - ``buffer.x``
     - Streaming a sequence through a buffer with a zero sentinel.
   * - ``sieve.x``
     - Concurrent prime sieve (a pipeline of filter processes).
   * - ``farm.x``
     - Worker farm: distributor → workers → collector (fan-out / fan-in).
   * - ``reduce.x``
     - Parallel sum over a binary tree of cores.
   * - ``scan.x``
     - Parallel prefix sum (scan) along a line of cores.
   * - ``stencil.x``
     - 1D nearest-neighbour halo exchange with bidirectional channels.
   * - ``mergesort.x``
     - Divide-and-conquer parallel sort with a stream merge.
   * - ``horner.x``
     - Systolic polynomial evaluation (a multiply-accumulate pipeline).

Two programs in full
--------------------

The Fibonacci program, a compact recursive sequential example:

.. literalinclude:: ../../examples/fib.x
   :language: text

The three-stage pipeline, a minimal concurrent example:

.. literalinclude:: ../../examples/pipe.x
   :language: text
