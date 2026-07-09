Lexical analyser
================

The lexical analyser turns the source text into a stream of tokens. Each call
to ``getNextToken`` consumes characters from the input and returns the next
``xcmp::Token``, recording any associated value (an identifier name, an integer
value, or a string) so that the parser can retrieve it. The token kinds and the
lexer live in ``src/xcmp.hpp``; the shared character-stream machinery is in
``src/lexer.hpp``.

The character-stream base
-------------------------

Both the assembler and the X compiler share a common scanning layer,
``hexlex::LexerBase<TokenT>`` in ``src/lexer.hpp``. It owns the input stream
(opened from a file with ``openFile`` or from memory with ``loadBuffer``),
tracks the current line and column for error reporting, and provides the
low-level helpers ``readChar``, ``skipWhitespace``, ``readIdentifier`` and
``readDecInt``. A language-specific lexer derives from it and supplies the
token-recognition logic by overriding ``readToken``:

.. literalinclude:: ../../src/lexer.hpp
   :language: cpp
   :lines: 85-91

The name table and keywords
---------------------------

``xcmp::Lexer`` keeps a ``TokenTable`` mapping identifier strings to tokens.
The language keywords (``and``, ``array``, ``chan``, ``do``, ``if``, ``proc``,
``while``, and the rest) are pre-loaded into the table by ``declareKeywords``
when the lexer is constructed. When an alphabetic character is seen,
``readIdentifier`` scans the whole word and ``TokenTable::lookup`` is used to
classify it: a keyword returns its reserved ``Token``, while any other word is
recorded as ``Token::IDENTIFIER`` (and the name is retrievable with
``getIdentifier``). There is no separate symbol/name table at this stage —
this single keyword table is all the lexer needs; meaning is attached to names
later, during :doc:`symbol-table construction <translator>`.

Numbers, characters and strings
-------------------------------

``readToken`` recognises three flavours of literal:

* **Numbers.** A leading digit is scanned as a decimal integer by ``readDecInt``;
  a ``#`` prefix selects a hexadecimal integer (``readHexInt``). Both yield
  ``Token::NUMBER`` with the value available from ``getNumber``.

* **Character constants.** A single-quoted character is read by
  ``readCharConst``, which also handles the escape sequences ``\\``, ``\'``,
  ``\"``, ``\t``, ``\r`` and ``\n``. A character constant produces a
  ``Token::NUMBER`` carrying the character's code.

* **Strings.** A double-quoted string is read by ``readString`` (a sequence of
  ``readCharConst`` calls) and produces a ``Token::STRING``; the text is
  retrievable with ``getString``.

Punctuation and operators are handled by a ``switch`` on the current character.
Multi-character tokens are recognised by lookahead: ``<`` and ``<=``, ``>`` and
``>=``, ``~`` and ``~=``, and ``:=`` (a bare ``:`` is an error). Comments begin
with ``|`` and run to the end of the line, after which lexing continues
recursively.

Relationship to the original
-----------------------------

In the self-hosting compiler ``examples/xhexb.x`` the same job is done by a
group of procedures: ``rdline`` and ``rch`` read the input a line and a
character at a time, ``readnumber`` scans integer literals in a given base,
``readstring`` collects string literals, and ``nextsymbol`` is the main
dispatch that classifies the next symbol (keywords are installed by
``declsyswords``). The C++ ``readToken`` plays the role of ``nextsymbol``, and
``hexlex::LexerBase`` subsumes ``rch``/``rdline``.

The corresponding source-language rules — what constitutes an identifier, a
number, a string and the comment syntax — are described in
:doc:`../language/lexical`.
