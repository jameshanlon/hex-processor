=============
Hex Processor
=============

.. toctree::
   :maxdepth: 2
   :caption: Contents:


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`


Hex tools
=========

Simulator (``hexsim``)
----------------------

Blah.

Assembler (``hexasm``)
----------------------

Blah

Compiler (``xcmp``)
-------------------

Blah.

Self-hosting compiler (``xhexb``)
---------------------------------

Blah.


Hardware implemenation
======================

Blah.

Hex architecture
================

.. LDAM  areg := mem[oreg]   Load from memory with an absolute address into areg
.. LDBM  breg := mem[oreg]   Load from memory with an absolute address from memory
.. into breg
.. STAM  mem[oreg] := areg   Store absolute from areg
.. LDAC  areg := oreg  Load absolute into areg
.. LDBC  breg := oreg  Load absolute into breg
.. LDAP  areg := pc + oreg   Load program counter-relative address into areg
.. LDAI  areg := mem[areg + oreg]  Load from memory with base and offset into areg
.. LDBI  breg := mem[breg + oreg]  Load from memory with base and offset into breg
.. STAI  mem[breg + oreg] := areg  Store to memory with base and offset from areg
.. BR  pc := pc + oreg   Branch relative
.. BRZ   if areg = 0: pc := pc + oreg  Conditional branch relative on areg being
.. zero
.. BRN   if areg < 0: pc := pc + oreg  Conditional branch relative on areg being
.. negative
.. BRB   pc := breg  Absolute branch
.. OPR     Inter-register operation
.. ADD   areg := areg + breg   Add areg and breg and set areg to the result
.. SUB   areg := areg + breg   Subtract areg and breg and set areg to the result
.. SVC     System call

Compilation
===========

Procedure calling
-----------------

Caller:

- ``LDAP <return address>`` (label following next ``BR``).
- ``BR <label>`` to procedure entry point.

Callee:

- Procedure entry stores the return address in ``areg``
- Extend stack to create callee frame.
- Execute callee body.
- Contract stack to delete callee frame.
- Exit loads return address into ``breg``.
- ``BRB`` to branch back to caller.

Caller:

- Continue execution after procedure call.

Stack memory layout:

```
Callee frame: 0 Return address (written by callee)
              1 Return value
              2 Actual 0
              3 Actual 1
              4 Actual 2
              5 Temporary 0
              6 Temporary 1
              7 Temporary 2
Caller frame: 8 Return address (written by callee)
              9 Return value
                Actual 0
                Actual 1
                Actual 2
                Temporary 0
                Temporary 1
                Temporary 2
                ...
```
