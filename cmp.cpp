#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <boost/format.hpp>

#include "Hex.hpp"

// A compiler for the X language, based on xhexb.x and with inspiration from
// the LLVM Kaleidoscope tutorial.
//   http://people.cs.bris.ac.uk/~dave/xarmdoc.pdf
//   https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html


//===---------------------------------------------------------------------===//
// Lexer
//===---------------------------------------------------------------------===//

enum class Token {
  NONE = 0,
  IDENTIFIER,
  NUMBER,
  LBRACKET,
  RBRACKET,
  LPAREN,
  RPAREN,
  IF,
  THEN,
  ELSE,
  WHILE,
  DO,
  ASS,
  SKIP,
  BEGIN,
  END,
  SEMICOLON,
  COMMA,
  VAR,
  ARRAY,
  PROC,
  FUNC,
  IS,
  STOP,
  NOT,
  NEG,
  VAL,
  STRING,
  TRUE,
  FALSE,
  RETURN,
  PLUS,
  MINUS,
  OR,
  AND,
  EQ,
  NE,
  LS,
  LE,
  GR,
  GE,
  END_OF_FILE
};

static const char *tokenEnumStr(Token token) {
  switch (token) {
  case Token::NONE:        return "NONE";
  case Token::IDENTIFIER:  return "IDENTIFIER";
  case Token::NUMBER:      return "NUMBER";
  case Token::LBRACKET:    return "[";
  case Token::RBRACKET:    return "]";
  case Token::LPAREN:      return "(";
  case Token::RPAREN:      return ")";
  case Token::IF:          return "if";
  case Token::THEN:        return "then";
  case Token::ELSE:        return "else";
  case Token::WHILE:       return "while";
  case Token::DO:          return "do";
  case Token::ASS:         return ":=";
  case Token::SKIP:        return "skip";
  case Token::BEGIN:       return "begin";
  case Token::END:         return "end";
  case Token::SEMICOLON:   return ";";
  case Token::COMMA:       return ",";
  case Token::VAR:         return "var";
  case Token::ARRAY:       return "array";
  case Token::PROC:        return "proc";
  case Token::FUNC:        return "func";
  case Token::IS:          return "is";
  case Token::STOP:        return "stop";
  case Token::NOT:         return "not";
  case Token::NEG:         return "-";
  case Token::VAL:         return "val";
  case Token::STRING:      return "string";
  case Token::TRUE:        return "true";
  case Token::FALSE:       return "false";
  case Token::RETURN:      return "return";
  case Token::PLUS:        return "+";
  case Token::MINUS:       return "-";
  case Token::OR:          return "or";
  case Token::AND:         return "and";
  case Token::EQ:          return "=";
  case Token::NE:          return "~=";
  case Token::LS:          return "<";
  case Token::LE:          return "<=";
  case Token::GR:          return ">";
  case Token::GE:          return ">=";
  case Token::END_OF_FILE: return "END_OF_FILE";
  default:
    throw std::runtime_error(std::string("unexpected token: ")+std::to_string(static_cast<int>(token)));
  }
};

bool isBinaryOp(Token token) {
  switch (token) {
  case Token::PLUS:
  case Token::MINUS:
  case Token::OR:
  case Token::AND:
  case Token::EQ:
  case Token::NE:
  case Token::LS:
  case Token::LE:
  case Token::GR:
  case Token::GE:
    return true;
  default:
    return false;
  }
}

class Table {
  std::map<std::string, Token> table;

public:
  void insert(const std::string &name, const Token token) {
    table.insert(std::make_pair(name, token));
  }

  /// Lookup a token type by identifier.
  Token lookup(const std::string &name) {
    auto it = table.find(name);
    if(it != table.end()) {
      return it->second;
    }
    table.insert(std::make_pair(name, Token::IDENTIFIER));
    return Token::IDENTIFIER;
  }
};

class Lexer {

  Table         table;
  std::ifstream file;
  char          lastChar;
  std::string   identifier;
  std::string   string;
  unsigned      value;
  Token         lastToken;
  size_t        currentLineNumber;
  std::string   currentLine;

  void declareKeywords() {
    table.insert("and",    Token::AND);
    table.insert("array",  Token::ARRAY);
    table.insert("do",     Token::DO);
    table.insert("else",   Token::ELSE);
    table.insert("false",  Token::FALSE);
    table.insert("func",   Token::FUNC);
    table.insert("if",     Token::IF);
    table.insert("is",     Token::IS);
    table.insert("or",     Token::OR);
    table.insert("proc",   Token::PROC);
    table.insert("return", Token::RETURN);
    table.insert("skip",   Token::SKIP);
    table.insert("stop",   Token::STOP);
    table.insert("then",   Token::THEN);
    table.insert("true",   Token::TRUE);
    table.insert("val",    Token::VAL);
    table.insert("var",    Token::VAR);
    table.insert("while",  Token::WHILE);
  }

  int readChar() {
    file.get(lastChar);
    currentLine += lastChar;
    if (file.eof()) {
      lastChar = EOF;
    }
    return lastChar;
  }

   void readDecInt() {
      std::string number(1, lastChar);
      while (std::isdigit(readChar())) {
        number += lastChar;
      }
      value = std::strtoul(number.c_str(), nullptr, 10);
   }

  void readHexInt() {
    std::string number(1, lastChar);
    do {
      number += readChar();
    } while(('0' <= lastChar && lastChar <= '9')
         || ('a' <= lastChar && lastChar <= 'z')
         || ('A' <= lastChar && lastChar <= 'Z'));
    value = std::strtoul(number.c_str(), nullptr, 16);
  }

  char readCharConst() {
    char ch;
    if (lastChar == '\\') {
      // Handle escape characters.
      readChar();
      switch (lastChar) {
      case '\\': ch = '\\'; break;
      case '\'': ch = '\''; break;
      case '"':  ch = '"';  break;
      case 't':  ch = '\t'; break;
      case 'r':  ch = '\r'; break;
      case 'n':  ch = '\n'; break;
      default:
          throw std::runtime_error("bad character constant");
      }
    } else {
      ch = lastChar;
    }
    readChar();
    return ch;
  }

  void readString() {
    string.clear();
    while (lastChar != '"' && lastChar != EOF) {
      string += readCharConst();
    }
  }

  Token readToken() {
    // Skip whitespace.
    while (std::isspace(lastChar)) {
      if (lastChar == '\n') {
        currentLineNumber++;
        currentLine.clear();
      }
      readChar();
    }
    // Comment.
    if (lastChar == '|') {
      do {
        readChar();
      } while (lastChar != EOF && lastChar != '\n');
      if (lastChar == '\n') {
        currentLineNumber++;
        currentLine.clear();
        readChar();
      }
      return readToken();
    }
    // Identifier.
    if (std::isalpha(lastChar)) {
      identifier = std::string(1, lastChar);
      while (std::isalnum(readChar()) || lastChar == '_') {
        identifier += lastChar;
      }
      return table.lookup(identifier);
    }
    // Decimal number.
    if (std::isdigit(lastChar)) {
      readDecInt();
      return Token::NUMBER;
    }
    // Hexdecimal number.
    if (lastChar == '#') {
      readHexInt();
      return Token::NUMBER;
    }
    Token token;
    switch(lastChar) {
    case '[': token = Token::LBRACKET;  break;
    case ']': token = Token::RBRACKET;  break;
    case '(': token = Token::LPAREN;    break;
    case ')': token = Token::RPAREN;    break;
    case '{': token = Token::BEGIN;     break;
    case '}': token = Token::END;       break;
    case ';': token = Token::SEMICOLON; break;
    case ',': token = Token::COMMA;     break;
    case '+': token = Token::PLUS;      break;
    case '-': token = Token::MINUS;     break;
    case '=': token = Token::EQ;        break;
    case '<':
      if (readChar() == '=') {
        token = Token::LE;
      } else {
        token = Token::LS;
      }
      break;
    case '>':
      if (readChar() == '=') {
        token = Token::GE;
      } else {
        token = Token::GR;
      }
      break;
    case '~':
      if (readChar() == '=') {
        token = Token::NE;
      } else {
        token = Token::NOT;
      }
      break;
    case ':':
      if (readChar() == '=') {
        token = Token::ASS;
      } else {
        throw std::runtime_error("'=' expected");
      }
      break;
    case '\'':
      readChar();
      value = readCharConst();
      token = Token::NUMBER;
      if (lastChar != '\'') {
        throw std::runtime_error("expected ' after char constant");
      }
      break;
    case '\"':
      readChar();
      readString();
      token = Token::STRING;
      if (lastChar != '"') {
        throw std::runtime_error("expected \" after string");
      }
      break;
    case EOF:
      file.close();
      token = Token::END_OF_FILE;
      break;
    default:
      throw std::runtime_error("unexpected character");
    }
    readChar();
    return token;
  }

public:

  Lexer() : currentLineNumber(0) {
    declareKeywords();
  }

  Token getNextToken() {
    return lastToken = readToken();
  }

  void openFile(const char *filename) {
    file.open(filename, std::ifstream::in);
    if (!file.is_open()) {
      throw std::runtime_error("could not open file");
    }
    readChar();
  }

  const std::string &getIdentifier() const { return identifier; }
  int getNumber() const { return value; }
  const std::string &getString() const { return string; }
  Token getLastToken() const { return lastToken; }
  size_t getLineNumber() const { return currentLineNumber; }
  const std::string &getLine() const { return currentLine; }
};

//===---------------------------------------------------------------------===//
// AST
//===---------------------------------------------------------------------===//

// Concrete AstNode forward references.
class Proc;
class Program;
class ArrayDecl;
class ValDecl;
class VarDecl;
class BinaryOp;
class UnaryOp;
class String;
class Boolean;
class Number;
class Call;
class ArraySubscript;
class VarRef;

/// A visitor base class for the AST.
struct AstVisitor {
  virtual void visit(Program&) = 0;
  virtual void visit(Proc&) = 0;
  virtual void visit(ArrayDecl&) = 0;
  virtual void visit(VarDecl&) = 0;
  virtual void visit(ValDecl&) = 0;
  virtual void visit(BinaryOp&) = 0;
  virtual void visit(UnaryOp&) = 0;
  virtual void visit(String&) = 0;
  virtual void visit(Boolean&) = 0;
  virtual void visit(Number&) = 0;
  virtual void visit(Call&) = 0;
  virtual void visit(ArraySubscript&) = 0;
  virtual void visit(VarRef&) = 0;
};

/// AST node base class.
class AstNode {
public:
  virtual ~AstNode() = default;
  virtual void accept(AstVisitor* visitor) = 0;
};

// Expressions ============================================================= //

class Expr : public AstNode {
public:
  Expr() {}
};

class VarRef : public Expr {
  std::string name;
  std::unique_ptr<Expr> expr;
public:
  VarRef(std::string name) : name(name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
  }
};

class ArraySubscript : public Expr {
  std::string name;
  std::unique_ptr<Expr> expr;
public:
  ArraySubscript(std::string name, std::unique_ptr<Expr> expr) :
      name(name), expr(std::move(expr)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
    expr->accept(visitor);
  }
};

class Call : public Expr {
  std::string name;
  std::vector<std::unique_ptr<Expr>> args;
public:
  Call(std::string name) : name(name) {}
  Call(std::string name, std::vector<std::unique_ptr<Expr>> args) :
      name(name), args(std::move(args)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
    for (auto &arg : args) {
      arg->accept(visitor);
    }
  }
};

class Number : public Expr {
  unsigned value;
public:
  Number(unsigned value) : value(value) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
  }
  unsigned getValue() const { return value; }
};

class Boolean : public Expr {
  bool value;
public:
  Boolean(bool value) : value(value) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
  }
  bool getValue() const { return value; }
};

class String : public Expr {
  std::string value;
public:
  String(std::string value) : value(value) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
  }
  std::string getValue() const { return value; }
};

class UnaryOp : public Expr {
  Token op;
  std::unique_ptr<Expr> element;
public:
  UnaryOp(Token op, std::unique_ptr<Expr> element) :
      op(op), element(std::move(element)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
    element->accept(visitor);
  }
  Token getOp() const { return op; }
};

class BinaryOp : public Expr {
  Token op;
  std::unique_ptr<Expr> LHS, RHS;
public:
  BinaryOp(Token op, std::unique_ptr<Expr> LHS, std::unique_ptr<Expr> RHS) :
      op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
    LHS->accept(visitor);
    RHS->accept(visitor);
  }
  Token getOp() const { return op; }
};

// Declarations ============================================================ //

class Decl : public AstNode {
  std::string name;
public:
  Decl(std::string name) : name(name) {}
  std::string getName() const { return name; }
};

class ValDecl : public Decl {
  std::unique_ptr<Expr> expr;
public:
  ValDecl(std::string name, std::unique_ptr<Expr> expr) :
      Decl(name), expr(std::move(expr)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
    expr->accept(visitor);
  }
};

class VarDecl : public Decl {
public:
  VarDecl(std::string name) : Decl(name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
  }
};

class ArrayDecl : public Decl {
  std::unique_ptr<Expr> expr;
public:
  ArrayDecl(std::string name, std::unique_ptr<Expr> expr) :
      Decl(name), expr(std::move(expr)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
    expr->accept(visitor);
  }
};

// Procedures and functions ================================================ //

class Proc : public AstNode {
  std::string name;
public:
  Proc() {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
  }
  std::string getName() const { return name; }
};

class Program : public AstNode {
  std::vector<std::unique_ptr<Decl>> globals;
  std::vector<std::unique_ptr<Proc>> procs;

public:
  void addGlobalDecl(std::unique_ptr<Decl> decl) {
    globals.push_back(std::move(decl));
  }
  void addProcDecl(std::unique_ptr<Proc> proc) {
    procs.push_back(std::move(proc));
  }
  virtual void accept(AstVisitor *visitor) override {
    visitor->visit(*this);
    for (auto &decl : globals) {
      decl->accept(visitor);
    }
    //for (auto &proc : procs) {
    //  proc->accept(visitor);
    //}
  }
};

// AST printer visitor ===================================================== //

class AstPrinter : public AstVisitor {
  std::ostream outs;
  //unsigned indent;
public:
  AstPrinter(std::ostream& outs = std::cout) :
      outs(outs.rdbuf())/*, indent(0)*/ {}
  virtual void visit(Program &decl) override {
    outs << "program\n";
  };
  virtual void visit(Proc &decl) override {
    outs << "proc\n";
  };
  virtual void visit(ArrayDecl &decl) override {
    outs << "arraydecl\n";
  };
  virtual void visit(VarDecl &decl) override {
    outs << "vardecl\n";
  };
  virtual void visit(ValDecl &decl) override {
    outs << "valdecl\n";
  };
  virtual void visit(BinaryOp &decl) override {
    outs << "binaryop\n";
  };
  virtual void visit(UnaryOp &decl) override {
    outs << "unaryop\n";
  };
  virtual void visit(String &decl) override {
    outs << "unaryop\n";
  };
  virtual void visit(Boolean &decl) override {
    outs << "boolean\n";
  };
  virtual void visit(Number &decl) override {
    outs << "number\n";
  };
  virtual void visit(Call &decl) override {
    outs << "call\n";
  };
  virtual void visit(ArraySubscript &decl) override {
    outs << "arraysubscript\n";
  };
  virtual void visit(VarRef &decl) override {
    outs << "varref\n";
  };
};


//===---------------------------------------------------------------------===//
// Parser
//===---------------------------------------------------------------------===//

class Parser {
  Lexer &lexer;

  /// Expect the given last token, otherwise raise an error.
  void expectLast(Token token) const {
    if (token != lexer.getLastToken()) {
      throw std::runtime_error(std::string("expected ")+tokenEnumStr(token));
    }
  }

  /// Expect the given next token.
  void expectNext(Token token) const {
    //lexer.getNextToken();
    expectLast(token);
  }

  int parseInteger() {
    if (lexer.getLastToken() == Token::MINUS) {
       expectNext(Token::NUMBER);
       return -lexer.getNumber();
    }
    expectLast(Token::NUMBER);
    return lexer.getNumber();
  }

  /// identifier
  std::string parseIdentifier() {
    if (lexer.getLastToken() == Token::IDENTIFIER) {
      lexer.getNextToken();
      return lexer.getIdentifier();
    } else {
      throw std::runtime_error("name expected");
    }
  }

  /// Associative operators can be chained (eg a + b + c + d).
  bool isAssociative(Token op) const {
    return op == Token::AND ||
           op == Token::OR ||
           op == Token::PLUS;
  }

  /// There is no operator associativity, so chains of associative operators
  /// are allowed, but otherwise binary expressions must be explicity bracketed.
  /// binary-op-RHS :=
  ///   <binary-op> <element> <binary-op>
  ///   <element>
  std::unique_ptr<Expr> parseBinOpRHS(Token op) {
    auto element = parseElement();
    if (isAssociative(op) && op == lexer.getLastToken()) {
      auto RHS = parseBinOpRHS(op);
      return std::make_unique<BinaryOp>(op, std::move(element), std::move(RHS));
    } else {
      return element;
    }
  }

  /// expression :=
  ///   "-" <element>
  ///   "~" <element>
  ///   <element> <binary-op-RHS>
  ///   <element>
  std::unique_ptr<Expr> parseExpr() {
    // Unary operations.
    if (lexer.getLastToken() == Token::MINUS) {
      auto element = parseElement();
      return std::make_unique<UnaryOp>(Token::MINUS, std::move(element));
    }
    if (lexer.getLastToken() == Token::NOT) {
      auto element = parseElement();
      return std::make_unique<UnaryOp>(Token::NOT, std::move(element));
    }
    auto element = parseElement();
    if (isBinaryOp(lexer.getLastToken())) {
      // Binary operation.
      auto op = lexer.getLastToken();
      auto RHS = parseBinOpRHS(op);
      return std::make_unique<BinaryOp>(op, std::move(element), std::move(RHS));
    }
    // Otherwise just return an element.
    return element;
  }

  /// expression-list :=
  ///   <expr> [ "," <expr> ]
  std::vector<std::unique_ptr<Expr>> parseExprList() {
    std::vector<std::unique_ptr<Expr>> exprList;
    do {
      exprList.push_back(parseExpr());
    } while (lexer.getLastToken() == Token::COMMA);
    return exprList;
  }

  /// element :=
  ///   <identifier>
  ///   <identifier> "[" <expr> "]"
  ///   <identifier> "(" <expr-list> ")"
  ///   <number>
  ///   <string>
  ///   "true"
  ///   "false"
  ///   "(" <expr> ")"
  std::unique_ptr<Expr> parseElement() {
    switch (lexer.getLastToken()) {
    case Token::IDENTIFIER: {
      auto name = parseIdentifier();
      // Array subscript.
      if (lexer.getLastToken() == Token::LBRACKET) {
        lexer.getNextToken();
        auto expr = parseExpr();
        expectNext(Token::RBRACKET);
        return std::make_unique<ArraySubscript>(name, std::move(expr));
      // Procedure call.
      } else if (lexer.getLastToken() == Token::LPAREN) {
        if (lexer.getNextToken() == Token::RPAREN) {
          return std::make_unique<Call>(name);
        } else {
          auto exprList = parseExprList();
          expectNext(Token::RPAREN);
          return std::make_unique<Call>(name, std::move(exprList));
        }
      // Variable reference.
      } else {
        return std::make_unique<VarRef>(name);
      }
    }
    case Token::NUMBER:
      return std::make_unique<Number>(lexer.getNumber());
    case Token::STRING:
      return std::make_unique<String>(lexer.getString());
    case Token::TRUE:
      return std::make_unique<Boolean>(true);
    case Token::FALSE:
      return std::make_unique<Boolean>(false);
    case Token::LPAREN: {
      lexer.getNextToken();
      auto expr = parseExpr();
      expectNext(Token::RPAREN);
      return expr;
    }
    default:
      throw std::runtime_error("in expression");
    }
  }

  /// declaration :=
  ///   "val" <identifier> "=" <expr> ";"
  ///   "var" <identifier> ";"
  ///   "array" <identifier> "[" <expr> "]" ";"
  std::unique_ptr<Decl> parseDecl() {
    switch (lexer.getLastToken()) {
    case Token::VAL: {
      lexer.getNextToken();
      auto name = parseIdentifier();
      expectNext(Token::EQ);
      auto expr = parseExpr();
      expectNext(Token::SEMICOLON);
      return std::make_unique<ValDecl>(name, std::move(expr));
    }
    case Token::VAR: {
      lexer.getNextToken();
      auto name = parseIdentifier();
      expectNext(Token::SEMICOLON);
      return std::make_unique<VarDecl>(name);
    }
    case Token::ARRAY: {
      lexer.getNextToken();
      auto name = parseIdentifier();
      expectNext(Token::LBRACKET);
      auto expr = parseExpr();
      expectNext(Token::RBRACKET);
      expectNext(Token::SEMICOLON);
      return std::make_unique<ArrayDecl>(name, std::move(expr));
    }
    default:
      throw std::runtime_error("invalid declaration");
    }
  }

  std::unique_ptr<Proc> parseProcDecl() {
    return std::make_unique<Proc>();
  }

public:
  Parser(Lexer &lexer) : lexer(lexer) {}

  Program parseProgram() {
    Program program;
    lexer.getNextToken();
    while (lexer.getLastToken() != Token::END_OF_FILE &&
           (lexer.getLastToken() == Token::VAL ||
            lexer.getLastToken() == Token::VAR ||
            lexer.getLastToken() == Token::ARRAY)) {
      program.addGlobalDecl(parseDecl());
    }
    while (lexer.getNextToken() != Token::END_OF_FILE) {
      program.addProcDecl(parseProcDecl());
    }
    return program;
  }
};

//===---------------------------------------------------------------------===//
// Driver
//===---------------------------------------------------------------------===//

static void help(const char *argv[]) {
  std::cout << "X compiler\n\n";
  std::cout << "Usage: " << argv[0] << " file\n\n";
  std::cout << "Positional arguments:\n";
  std::cout << "  file              A source file to compile\n\n";
  std::cout << "Optional arguments:\n";
  std::cout << "  -h,--help         Display this message\n";
  std::cout << "  --tokens          Tokenise the input only\n";
  std::cout << "  --tree            Display the syntax tree only\n";
  std::cout << "  -o,--output file  Specify a file for output (default a.S)\n";
}

int main(int argc, const char *argv[]) {
  Lexer lexer;
  Parser parser(lexer);

  try {

    // Handle arguments.
    bool tokensOnly = false;
    bool treeOnly = false;
    const char *filename = nullptr;
    const char *outputFilename = "a.out";
    for (unsigned i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0) {
        help(argv);
        std::exit(1);
      } else if (std::strcmp(argv[i], "--tokens") == 0) {
        tokensOnly = true;
      } else if (std::strcmp(argv[i], "--tree") == 0) {
        treeOnly = true;
      } else if (std::strcmp(argv[i], "--output") == 0 ||
                 std::strcmp(argv[i], "-o") == 0) {
        outputFilename = argv[++i];
      } else if (argv[i][0] == '-') {
          throw std::runtime_error(std::string("unrecognised argument: ")+argv[i]);
      } else {
        if (!filename) {
          filename = argv[i];
        } else {
          throw std::runtime_error("cannot specify more than one file");
        }
      }
    }

    // A file must be specified.
    if (!filename) {
      help(argv);
      std::exit(1);
    }

    // Open the file.
    lexer.openFile(filename);

    // Tokenise only.
    if (tokensOnly && !treeOnly) {
      while (true) {
        switch (lexer.getNextToken()) {
          case Token::IDENTIFIER:
            std::cout << lexer.getIdentifier() << "\n";
            break;
          case Token::NUMBER:
            std::cout << lexer.getNumber() << "\n";
            break;
          case Token::STRING:
            std::cout << lexer.getString() << "\n";
            break;
          case Token::END_OF_FILE:
            std::cout << "EOF\n";
            std::exit(0);
          default:
            std::cout << tokenEnumStr(lexer.getLastToken()) << "\n";
            break;
        }
      }
      return 0;
    }

    // Parse the program.
    auto ast = parser.parseProgram();

    // Parse and print program only.
    if (treeOnly) {
      AstPrinter printer;
      ast.accept(&printer);
      return 0;
    }

  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    std::cerr << "  " << lexer.getLineNumber() << ": " << lexer.getLine() << "\n";
    std::exit(1);
  }
  return 0;
}
