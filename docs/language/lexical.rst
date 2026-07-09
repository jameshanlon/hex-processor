Lexical structure
=================

This page describes how the source text of an X program is broken into tokens.
The rules below match the lexer in ``src/xcmp.hpp`` (class ``Lexer``), which is
the authority where this description and the original 2014 language note differ.

Character set
-------------

Source is plain ASCII text. Whitespace — spaces, tabs and newlines — separates
tokens but is otherwise insignificant; it may be used freely to lay out a
program. There is no line-continuation or significant-indentation rule.

Names
-----

A *name* (identifier) begins with an alphabetic character and continues with any
number of alphabetic characters, decimal digits and underscores. Names are
**case sensitive**, so ``foo``, ``Foo`` and ``FOO`` are distinct. Examples drawn
from the example programs include ``div_x``, ``foo_BAR`` and ``bytesperword``.

A name that matches a reserved word is treated as that keyword rather than as an
identifier. The reserved words are:

.. code-block:: text

   and  array  chan  do    else   false  func  if   is
   or   par    proc  return skip   stop   then  true val  var  while

Literals
--------

**Integer literals** are word-sized. A decimal literal is a run of digits, for
example ``0``, ``10`` or ``256``. A hexadecimal literal is written with a
leading ``#`` followed by hexadecimal digits (``0``–``9``, ``a``–``f``,
``A``–``F``), for example ``#ff`` or ``#100``. (Note: the prefix is ``#``, not
``0x``.)

**Character constants** are written between single quotes, as in ``'P'`` or
``'\n'``. A character constant denotes the integer value of that one character,
so it can be used anywhere an integer is expected — ``out ! 'P'`` sends the
code for ``P``, and ``rem(n, 10) + '0'`` converts a digit to its ASCII
character.

**String literals** are written between double quotes, for example
``"hello world\n"``. A string is stored packed into consecutive words, with the
length of the string held in the low byte of the first word and the characters
following it; the ``prints`` procedure in ``examples/hello_prints.x`` shows how
this layout is read back at run time. Strings are passed to procedures as
``array`` arguments.

**Boolean literals** ``true`` and ``false`` are reserved words denoting the
usual truth values.

Comments
--------

A comment begins with a vertical bar ``|`` and runs to the end of the line; the
``|`` and everything after it on that line are ignored. (This is a single
end-of-line delimiter — the bar does not need to be closed by a matching bar.)
Comments are used both for whole-line headers and for trailing annotations:

.. code-block:: text

   | A token ring of three processors.
   { out ! 90;   | inject the token
     in ? v }    | wait for it to return

Escape sequences
----------------

Inside character and string literals, a backslash introduces an escape sequence.
The lexer recognises the following escapes; any other character after a
backslash is an error.

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Escape
     - Meaning
   * - ``\n``
     - Newline (line feed)
   * - ``\r``
     - Carriage return
   * - ``\t``
     - Horizontal tab
   * - ``\'``
     - Single quote
   * - ``\"``
     - Double quote
   * - ``\\``
     - Backslash

.. note::

   The original 2014 language note used the occam-style ``*n`` escape convention
   (``*n``, ``*t``, ``*'``, ``**``, ``*#hh``) and ``| ... |`` paired comments.
   The current compiler instead uses C-style backslash escapes and ``|``
   to-end-of-line comments as described above, and has no numeric ``*#hh``-style
   escape. Where this page and the old note disagree, the compiler wins.
