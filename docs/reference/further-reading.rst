Further reading
===============

The Hex project sits in a long line of work on small processors and small,
portable languages.  This page is a short, annotated guide to that lineage and
to the narrative companion to these reference docs.

Historical lineage
------------------

**The Transputer and "Simple 42".**
   The Hex architecture descends from the Transputer family — a processor
   designed around point-to-point communication and concurrency — and from the
   "Simple 42" teaching processor.  Hex keeps the Transputer's idea of a core
   that communicates over channels (the ``IN``/``OUT`` operations and the
   network of link slots) while paring the design back to something small
   enough to explain end to end.

**BCPL and the small-language tradition.**
   X stands in the tradition of BCPL, the small systems language created by
   Martin Richards.  BCPL pioneered the approach of a compact, typeless,
   close-to-the-machine language that is easy to retarget — the same approach X
   takes in compiling directly to Hex instructions.

**Occam and channel-based concurrency.**
   X's concurrency — ``par`` blocks and the ``!``/``?`` channel operators —
   follows Occam, the Transputer's language, also designed by David May.  As in
   Occam, processes share nothing and communicate only by synchronous message
   passing over channels.

**Bootstrapping and portability.**
   The compiler is written in its own language and bootstraps itself: a small
   compiler, portable by retargeting its code generator, that can rebuild
   itself from source.  This is the classic route to a self-hosting toolchain,
   and is described in :doc:`../compiler/bootstrapping`.

The narrative companion
-----------------------

The author's blog post **"From logic gates to a programming language using the
Hex architecture"** walks the whole stack as a single narrative — from logic
gates up through the processor, the X language and the self-hosting compiler.
It is the readable, story-shaped counterpart to this reference, and the best
starting point for understanding *why* the pieces fit together the way they do.

.. seealso::

   :doc:`../index` for the layered overview of the whole stack, and
   :doc:`primary-sources` for David May's original notes on Hex and X.
