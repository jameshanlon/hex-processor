Building
========

The project is built with CMake.  A configure step fetches the few external
dependencies, after which a normal build produces the six tools described in
:doc:`index` and installs them under a chosen prefix.

Dependencies
------------

* CMake 3.20 or newer.
* A C++20-capable compiler.
* Python 3 (for the integration tests and the documentation build).
* Verilator 5.0 or newer (optional; only needed for the RTL testbench
  ``hextb``).  If no suitable Verilator is found, a pinned version is fetched
  and built automatically.

All other libraries — ``fmt`` and Catch2 — are fetched automatically via
CMake's ``FetchContent``, so no manual installation is required.

On Ubuntu the system packages needed to bootstrap are:

.. code-block:: bash

   $ apt update && apt install build-essential cmake

On macOS with Homebrew:

.. code-block:: bash

   $ brew install cmake

Configuring and building
-------------------------

Configure into a ``build`` directory, build, and install:

.. code-block:: bash

   $ mkdir build
   $ cd build
   $ cmake .. -DCMAKE_BUILD_TYPE=Debug \
              -DCMAKE_INSTALL_PREFIX=$(pwd)/install \
              -DUSE_VERILATOR=OFF
   $ make -j8
   $ make install

This installs the tools into ``build/install/bin``; add that directory to your
``PATH`` to run them by name:

.. code-block:: bash

   $ export PATH=$(pwd)/install/bin:$PATH

Verilator and the RTL
---------------------

``-DUSE_VERILATOR`` defaults to ``ON``.  Drop the ``-DUSE_VERILATOR=OFF`` from
the configure line above to build the RTL testbench ``hextb`` as well.  When
Verilator is enabled, CMake uses a suitable system Verilator (5.0 or newer) if
it can find one — honouring ``-DVERILATOR_ROOT=<path>`` to point at a specific
installation — and otherwise fetches and builds a pinned version automatically:

.. code-block:: bash

   $ cmake .. -DCMAKE_BUILD_TYPE=Debug \
              -DCMAKE_INSTALL_PREFIX=$(pwd)/install \
              -DVERILATOR_ROOT=/opt/verilator

A recent Verilator is required for ``hextb``: the project targets the 5.x
series, and roughly 4.200 is the practical floor — older releases (such as the
4.038 shipped by some distributions) are too old to build the testbench.

Building the documentation
--------------------------

These Sphinx documents can be built two ways.

The first is through CMake.  Configure with ``-DBUILD_DOCS=ON`` and build the
``Sphinx`` target:

.. code-block:: bash

   $ cmake .. -DBUILD_DOCS=ON
   $ make Sphinx

The CMake docs target prefers the project's virtual-environment Sphinx (see
below) and builds with ``-W``, so documentation warnings are treated as build
errors.

The second is to drive Sphinx directly from a Python virtual environment, which
is convenient for iterating on the docs without reconfiguring CMake:

.. code-block:: bash

   $ python3 -m venv docs/_venv
   $ docs/_venv/bin/pip install -r docs/requirements.txt
   $ docs/_venv/bin/sphinx-build -b html -W --keep-going docs docs/_build/html

The ``-W`` flag turns warnings into errors and ``--keep-going`` reports all of
them rather than stopping at the first.  The HTML output is written to
``docs/_build/html``.
