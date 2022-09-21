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


### Hardware implemenation

  Blah.

  ### Hex tools

  ### Simulator (``hexsim``)

  Blah.

  ### Assembler (``hexasm``)

  Blah

  ### Compiler (``xcmp``)

  Blah.

  ### Self-hosting compiler (``xhexb``)

  Blah.

  #### Structure

  #### Procedure calling

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
