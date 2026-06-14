Memory layout and calling convention
=====================================

This page describes how the compiler arranges memory and how it implements
procedure and function calls. It documents what ``src/xcmp.hpp`` actually emits;
the relevant constants are defined near the top of the code-generation section
(``SP_OFFSET``, ``MAX_ADDRESS``, ``SP_LINK_VALUE_OFFSET``,
``SP_RETURN_VALUE_OFFSET``, ``FB_PARAM_OFFSET_FUNC``, ``FB_PARAM_OFFSET_PROC``).

The memory map
--------------

Word addresses run from low to high. The fixed low words and the data regions
are laid out as follows:

* **Word 0** — a ``BR`` to the program ``start`` label. Booting an image means
  jumping to word 0.
* **Word 1** — the stack pointer (``SP_OFFSET`` is ``1``). It is initialised by
  the compiler to a word emitted in the ``SP_VALUE`` slot, with the value
  ``MAX_ADDRESS - globalsOffset - 1`` (see ``LowerDirectives``), i.e. just below
  the global arrays.
* **Globals, constants and strings** — the data section. Each global ``var``
  is a ``DATA 0`` word with its own label (``visitPost(VarDecl&)``); large
  constants are pooled (``genConstPool``) and string literals are packed into
  words (``genString``), each with a label.
* **Program** — the emitted instructions.
* **Stack** — grows downward from the initial stack pointer.
* **Arrays** — global arrays are allocated at the very top of memory.
  ``visitPost(ArrayDecl&)`` allocates ``decl.getSize()`` words ending at
  ``MAX_ADDRESS``, and stores the array's base address into the global word that
  names it. A global array reference therefore loads a word whose contents are
  the array's address.

Constants that fit in a 16-bit operand are loaded directly with
``LDAC``/``LDBC`` (with PFIX/NFIX prefixes); larger constants are loaded from
the pooled data word with ``LDAM``/``LDBM`` (``genConst``).

The stack frame
---------------

A ``Frame`` (one per procedure/function) tracks the running and maximum frame
size and the procedure's exit label. Storage is addressed as offsets from the
*frame base*, which is the first word above the callee's frame — that is, the
boundary with the caller's frame. The fixed slots at the base are:

* offset 0 — the saved link (return) address (``SP_LINK_VALUE_OFFSET``);
* offset 1 — the return value slot, for functions (``SP_RETURN_VALUE_OFFSET``);
* the actual parameters, starting at ``FB_PARAM_OFFSET_PROC`` (``1``) for
  procedures and ``FB_PARAM_OFFSET_FUNC`` (``2``) for functions, assigned in
  order by ``FormalLocations``.

Local variables and arrays are allocated *below* the frame base (negative
offsets) by ``LocalDeclLocations``, growing the frame size as they go. Because
the prologue extends the stack pointer by the whole frame size, accesses are
emitted against the frame base as ``LDAI_FB``/``LDBI_FB``/``STAI_FB`` and then
lowered by ``LowerDirectives`` to plain ``LDAI``/``LDBI``/``STAI`` with the
offset recomputed as ``frameSize - 1 + fbOffset``.

Variable access then comes in two forms (``CodeBuffer::genVar``):

* **Global**: ``LDAM label`` (or ``LDBM label``) — a direct memory load by
  label.
* **Local**: ``LDAM 1`` (load the stack pointer from word 1) followed by an
  indexed ``LDAI`` at the frame-relative offset.

The calling sequence
--------------------

A call is built by ``genProcCall``/``genFuncCall`` (and ``genSysCall`` for
system calls). The caller evaluates the actual parameters and writes them into
the callee's parameter slots, just above the current stack pointer
(``loadActuals``). Arguments that themselves contain calls are evaluated first
into temporary stack words (``genCallActuals``) so that nested calls do not
clobber the parameter area before it is fully populated. The caller then loads a
link (return) address with ``LDAP`` and branches with ``BR`` to the callee;
``LDAP`` makes the return address PC-relative, which keeps calls position
independent. After a function call the result is read back from the return-value
slot with ``LDAM 1; LDAI 1``.

On entry, the **prologue** (emitted from the ``PROLOGUE`` directive in
``LowerDirectives``) saves the link address and opens the frame:

* ``LDBM 1; STAI 0`` — store the current stack pointer's link slot (offset 0);
* if the frame is non-empty, ``LDAC -frameSize; ADD; STAM 1`` — decrement the
  stack pointer by the frame size (the stack grows downward).

On exit, the **epilogue** (from the ``EPILOGUE`` directive) reverses this. For a
function it first stores the result (already in the A register) into the
caller-visible return-value slot with ``LDBM 1; STAI frameSize+1``; then, for
both procedures and functions, if the frame is non-empty it contracts the stack
pointer (``LDAC frameSize; ADD; STAM 1``), reloads the (unadjusted) stack
pointer, and returns with ``LDBI frameSize; OPR BRB`` — ``BRB`` branches to the
return address held in the B register. The procedure's exit label is the target
that ``ReturnStatement`` branches to, so all returns funnel through this single
epilogue.

The program is bootstrapped by ``CodeGen::visitPre(Program&)``, which emits the
word-0 branch, the ``SP_VALUE`` placeholder, and a small entry sequence that
branches-and-links to ``main`` and, on return, performs an exit system call.
The ``stop`` statement compiles to the same exit syscall.

See :doc:`../architecture/instruction-set` for the instructions used here and
:doc:`../architecture/registers` for the A, B and program-counter registers and
the role of ``BRB``.
