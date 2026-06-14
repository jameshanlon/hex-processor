Testing
=======

The project has three layers of tests: C++ unit tests written with Catch2, a
Python integration-test driver that exercises the tools end to end, and
Verilator testbenches for the RTL.  All of them are wired into CTest, so the
quickest way to run everything is from the build directory:

.. code-block:: bash

   $ ctest --output-on-failure

``--output-on-failure`` prints the captured output of any test that fails,
which is usually enough to see what went wrong.

C++ unit tests
--------------

The unit tests live in ``tests/unit/`` and are compiled into a single
``UnitTests`` executable.  Run them directly for fast feedback:

.. code-block:: bash

   $ ./UnitTests                  # run all unit tests
   $ ./UnitTests "<test name>"    # run a single named test

The test sources cover the assembler, the disassembler, the X language and its
programs, and the multi-core simulator:

.. list-table:: Unit-test sources
   :header-rows: 1
   :widths: 34 66

   * - File
     - Covers
   * - ``AssemblerTests.cpp``
     - The assembler: lexing, instruction selection and encoding.
   * - ``AssemblerProgramTests.cpp``
     - Whole assembly programs assembled and run end to end.
   * - ``DisassemblerTests.cpp``
     - The disassembler: decoding images back to listings.
   * - ``XLanguageTests.cpp``
     - X language front-end features (parsing and compilation).
   * - ``XProgramTests.cpp``
     - Whole X programs compiled and executed for their results.
   * - ``SimTests.cpp``
     - The simulator, including channels and multi-core message-passing
       networks.

The directory also contains a collection of ``.x`` programs (for example
``ackermann.x``, ``mergesort.x``, ``sieve.x`` and the self-hosting
``xhexb.x``) that the program-level tests compile and run.  ``TestContext.hpp``
provides the shared fixture used across the suites.

Python integration tests
-------------------------

The end-to-end integration tests are driven by ``tests/tests.py``, which
invokes the built tools to compile and execute programs and checks their
output:

.. code-block:: bash

   $ python3 ../tests/tests.py

These complement the unit tests by verifying the complete compile-and-run flow
through the installed binaries rather than the library internals.  The
assembly programs they reference live in ``tests/asm/`` (``hello.S``,
``hello_procedure.S``, ``exit0.S``, ``exit255.S`` and ``xhexb.S``).

RTL testbenches
---------------

The hardware tests are Verilator testbenches in ``tests/rtl/``:
``core_tb.cpp`` exercises a single processor core, ``router_tb.cpp`` and
``liu_tb.cpp`` exercise the router and link interface, and
``flit_layout.hpp`` holds the shared flit definitions they use.  These require
a Verilator build (see :doc:`building`) and are described in more detail in
:doc:`../hardware/testbench`.

.. seealso::

   :doc:`../hardware/testbench` for the structure of the RTL testbenches and
   how they drive the design.
