Grammar
=======

This page consolidates the grammar of X in one place. It is reconciled with the
recursive-descent parser in ``src/xcmp.hpp`` (and cross-checked against the
compiler written in X, ``examples/xhexb.x``); where it differs from the original
2014 language note, the differences are flagged below and the parser is taken as
authoritative.

The notation is informal EBNF: ``|`` separates alternatives, ``[ x ]`` means an
optional ``x``, and ``{ x }`` means zero or more repetitions of ``x``. Terminals
are quoted.

.. code-block:: text

   program        := global-decls proc-decls

   global-decls   := { global-decl }
   global-decl    := "val"   name "=" expression ";"
                   | "var"   name ";"
                   | "array" name "[" expression "]" ";"
                   | "chan"  name ";"

   proc-decls     := { proc-decl }
   proc-decl      := ( "proc" | "func" ) name "(" [ formals ] ")" "is"
                       local-decls statement

   local-decls    := { local-decl }
   local-decl     := "val"  name "=" expression ";"
                   | "var"  name ";"
                   | "chan" name ";"

   formals        := formal { "," formal }
   formal         := "val"   name
                   | "var"   name
                   | "array" name
                   | "chan"  name
                   | "proc"  name
                   | "func"  name

   statement      := "skip"
                   | "stop"
                   | "return" expression
                   | "if" expression "then" statement "else" statement
                   | "while" expression "do" statement
                   | "{" statement { ";" statement } "}"
                   | "par" "{" statement { ";" statement } "}"
                   | element ":=" expression
                   | element "!" expression
                   | element "?" element
                   | name   "(" [ expr-list ] ")"
                   | number "(" [ expr-list ] ")"

   expr-list      := expression { "," expression }

   expression     := "-" element
                   | "~" element
                   | element [ binary-op element { binary-op element } ]

   binary-op      := "+" | "-" | "and" | "or"
                   | "=" | "~=" | "<" | "<=" | ">" | ">="

   element        := name
                   | name "[" expression "]"
                   | name   "(" [ expr-list ] ")"
                   | number "(" [ expr-list ] ")"
                   | number
                   | string
                   | "true"
                   | "false"
                   | "(" expression ")"

   name           := alpha { alpha | digit | "_" }
   number         := digit { digit }                  | "#" hexdigit { hexdigit }
                   | "'" character "'"
   string         := '"' { character } '"'

Notes on the grammar
--------------------

* **No precedence; chaining only for one operator.** ``binary-op`` does not
  encode precedence levels. The parser will chain a repeated *associative*
  operator (``a + b + c``), but an expression that mixes different operators must
  be parenthesised. See :doc:`expressions`.

* **``par`` branches are restricted.** Syntactically a ``par`` block is a
  brace-delimited, semicolon-separated list of statements, but each branch must
  be a procedure call; the compiler rejects any other statement form as a
  branch. See :doc:`concurrency`.

* **Channel input and output** are statements (``element "!" expression`` and
  ``element "?" element``), distinguished from assignment by the ``!`` / ``?``
  operator after the leading element.

* **Calls double as statements and elements.** A ``name(...)`` or
  ``number(...)`` call is both a statement (a procedure or syscall call) and an
  element (a function or syscall result used in an expression). A statement that
  begins with a number must be a syscall.

* **No ``valof``.** The 2014 note expressed function results with a
  ``valof`` / ``return`` block. The parser has no ``valof``: a ``func`` body is
  an ordinary ``statement`` (preceded by optional local declarations) that
  yields its result with ``return``. See :doc:`procedures-functions`.

* **Comments and escapes** follow the lexer, not the original note: ``|``
  introduces a to-end-of-line comment, and string/character escapes are
  C-style (``\n``, ``\t``, ``\\`` …). See :doc:`lexical`.
