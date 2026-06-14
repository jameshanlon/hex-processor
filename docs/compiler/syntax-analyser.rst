Syntax analyser
===============

The syntax analyser reads the token stream and builds an abstract syntax tree
(AST). It is a hand-written *recursive-descent* parser, ``xcmp::Parser`` in
``src/xcmp.hpp``: each grammar production corresponds to a parsing method, and
the structure of those methods mirrors the structure of the
:doc:`grammar <../language/grammar>`.

Recursive descent
-----------------

``Parser`` holds a reference to the lexer and drives it one token at a time.
The helper ``expect`` checks that the current token is the one required and
advances past it (raising ``UnexpectedTokenError`` otherwise), and
``parseIdentifier`` reads a name. The entry point ``parseProgram`` parses the
global declarations followed by the procedure and function declarations:

.. literalinclude:: ../../src/xcmp.hpp
   :language: cpp
   :lines: 1961-1969

From there the methods descend through the grammar:
``parseGlobalDecls``/``parseLocalDecls`` and ``parseDecl`` for ``val``, ``var``,
``array`` and ``chan`` declarations; ``parseProcDecl`` for procedure and
function bodies (including their formals and local declarations);
``parseStatement``/``parseStatements`` for the statement forms; and
``parseExpr``/``parseElement`` for expressions.

Expressions are kept simple: there is no operator precedence. ``parseExpr``
reads a leading element, optionally negated by a unary ``-`` or ``~``, and at
most one binary operator. Chains are only permitted for the associative
operators ``+``, ``and`` and ``or`` (see ``isAssociative`` and
``parseBinOpRHS``); any other combination of operators must be explicitly
bracketed. ``parseElement`` handles the atoms: variable references, array
subscripts, procedure/function calls, numeric system calls, numbers, strings,
``true``/``false`` and parenthesised expressions.

The AST
-------

The tree is a hierarchy of C++ classes rooted at ``xcmp::AstNode``. This is a
significant departure from the original ``xhexb.x`` compiler, which represents
the tree as tagged vectors built by the ``cons1`` … ``cons4`` constructor
functions; in the C++ version each node kind is its own class with typed
fields. The main families are:

* **Expressions** (``Expr`` and subclasses): ``BinaryOpExpr``, ``UnaryOpExpr``,
  ``NumberExpr``, ``BooleanExpr``, ``StringExpr``, ``VarRefExpr``,
  ``ArraySubscriptExpr`` and ``CallExpr``. Every ``Expr`` carries an optional
  constant value, set later by constant propagation (``isConst``/``getValue``).

* **Declarations** (``Decl``): ``ValDecl``, ``VarDecl``, ``ArrayDecl`` and
  ``ChanDecl``.

* **Formals** (``Formal``): ``ValFormal``, ``VarFormal``, ``ArrayFormal``,
  ``ProcFormal``, ``FuncFormal`` and ``ChanFormal``.

* **Statements** (``Statement``): ``SkipStatement``, ``StopStatement``,
  ``ReturnStatement``, ``IfStatement``, ``WhileStatement``, ``SeqStatement``,
  ``CallStatement``, ``AssStatement``, ``ParStatement``, ``OutStatement`` and
  ``InStatement``.

* **Top level**: a ``Proc`` (used for both procedures and functions, with an
  ``isFunction`` flag) holds its formals, local declarations and body
  statement; a ``Program`` holds the global declarations and the list of
  ``Proc`` definitions.

For example, parsing a statement that begins with an identifier produces a
``CallStatement``, an ``OutStatement`` (``!``), an ``InStatement`` (``?``) or an
``AssStatement`` depending on what follows the element:

.. literalinclude:: ../../src/xcmp.hpp
   :language: cpp
   :lines: 1871-1895

The visitor pattern
-------------------

Each node implements ``accept(AstVisitor *)``, which calls the visitor's
``visitPre`` method, recurses into children, then calls ``visitPost``. All the
later passes — symbol-table construction, constant propagation, the expression
optimiser, code generation and the AST printer — are written as subclasses of
``AstVisitor``. A visitor can also replace an expression node in place (via
``setExprReplacement``), which is how the optimiser rewrites the tree. The
``--tree`` option of ``tools/xcmp.cpp`` runs the ``AstPrinter`` visitor to dump
the parsed tree.
