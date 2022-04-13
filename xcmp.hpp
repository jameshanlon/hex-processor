#ifndef X_CMP_HPP
#define X_CMP_HPP

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <stack>
#include <vector>
#include <map>
#include <boost/format.hpp>

#include "util.hpp"

// A compiler for the X language, based on xhexb.x and with inspiration from
// the LLVM Kaleidoscope tutorial.
//   http://people.cs.bris.ac.uk/~dave/xarmdoc.pdf
//   https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html

using Location = hexutil::Location;
using Error = hexutil::Error;

namespace xcmp {

//===---------------------------------------------------------------------===//
// Lexer tokens
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
  case Token::BEGIN:       return "{";
  case Token::END:         return "}";
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

static bool isBinaryOp(Token token) {
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

//===---------------------------------------------------------------------===//
// Exceptions
//===---------------------------------------------------------------------===//

struct CharConstError : public Error {
  CharConstError(Location location) : Error(location, "bad character constant") {}
};

struct TokenError : public Error {
  TokenError(Location location, std::string message) : Error(location, message) {}
};

struct UnexpectedTokenError : public Error {
  UnexpectedTokenError(Location location, Token expectedToken, Token gotToken) :
      Error(location, (boost::format("expected token %s, got %s") % tokenEnumStr(expectedToken) % tokenEnumStr(gotToken)).str()) {}
};

struct ExpectedNameError : public Error {
  ExpectedNameError(Location location, Token token) :
    Error(location, (boost::format("expected name but got %s") % tokenEnumStr(token)).str()) {}
};

struct ParserTokenError : public Error {
  ParserTokenError(Location location, std::string message, Token token) :
    Error(location, (boost::format("%s, got %s") % message % tokenEnumStr(token)).str()) {}
};

struct SemanticTokenError : public Error {
  SemanticTokenError(Location location, std::string message, Token token) :
    Error(location, (boost::format("%s, got %s") % message % tokenEnumStr(token)).str()) {}
};

struct UnknownSymbolError : public Error {
  UnknownSymbolError(Location location, std::string name) :
    Error(location, (boost::format("could not find symbol %s") % name).str()) {}
};

//===---------------------------------------------------------------------===//
// Lexer
//===---------------------------------------------------------------------===//

class TokenTable {
  std::map<std::string, Token> table;

public:
  void insert(const std::string &name, const Token token) {
    table.insert(std::make_pair(name, token));
  }

  /// Lookup a token type by identifier.
  Token lookup(const std::string &name) {
    auto it = table.find(name);
    if (it != table.end()) {
      return it->second;
    }
    table.insert(std::make_pair(name, Token::IDENTIFIER));
    return Token::IDENTIFIER;
  }
};

class Lexer {

  TokenTable                    table;
  std::unique_ptr<std::istream> file;
  char                          lastChar;
  std::string                   identifier;
  std::string                   string;
  unsigned                      value;
  Token                         lastToken;
  size_t                        currentLineNumber;
  size_t                        currentCharNumber;
  std::string                   currentLine;

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
    file->get(lastChar);
    currentLine += lastChar;
    if (file->eof()) {
      lastChar = EOF;
    }
    currentCharNumber++;
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
        throw CharConstError(getLocation());
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
        currentCharNumber = 0;
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
        currentCharNumber = 0;
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
    case '[': readChar(); token = Token::LBRACKET;  break;
    case ']': readChar(); token = Token::RBRACKET;  break;
    case '(': readChar(); token = Token::LPAREN;    break;
    case ')': readChar(); token = Token::RPAREN;    break;
    case '{': readChar(); token = Token::BEGIN;     break;
    case '}': readChar(); token = Token::END;       break;
    case ';': readChar(); token = Token::SEMICOLON; break;
    case ',': readChar(); token = Token::COMMA;     break;
    case '+': readChar(); token = Token::PLUS;      break;
    case '-': readChar(); token = Token::MINUS;     break;
    case '=': readChar(); token = Token::EQ;        break;
    case '<':
      if (readChar() == '=') {
        readChar();
        token = Token::LE;
      } else {
        token = Token::LS;
      }
      break;
    case '>':
      if (readChar() == '=') {
        readChar();
        token = Token::GE;
      } else {
        token = Token::GR;
      }
      break;
    case '~':
      if (readChar() == '=') {
        readChar();
        token = Token::NE;
      } else {
        token = Token::NOT;
      }
      break;
    case ':':
      if (readChar() == '=') {
        readChar();
        token = Token::ASS;
      } else {
        throw TokenError(getLocation(), "'=' expected");
      }
      break;
    case '\'':
      readChar();
      value = readCharConst();
      token = Token::NUMBER;
      if (lastChar != '\'') {
        throw TokenError(getLocation(), "expected ' after char constant");
      }
      readChar();
      break;
    case '\"':
      readChar();
      readString();
      token = Token::STRING;
      if (lastChar != '"') {
        throw TokenError(getLocation(), "expected \" after string");
      }
      readChar();
      break;
    case EOF:
      if (auto ifstream = dynamic_cast<std::ifstream*>(file.get())) {
        ifstream->close();
      }
      token = Token::END_OF_FILE;
      readChar();
      currentLine.clear();
      break;
    default:
      throw TokenError(getLocation(), std::string("unexpected character ")+lastChar);
    }
    return token;
  }

public:

  Lexer() : currentLineNumber(0), currentCharNumber(0) {
    declareKeywords();
  }

  Token getNextToken() {
    return lastToken = readToken();
  }

  /// Open a file using ifstream.
  void openFile(const char *filename) {
    auto ifstream = std::make_unique<std::ifstream>();
    ifstream->open(filename, std::ifstream::in);
    if (!ifstream->is_open()) {
      throw std::runtime_error("could not open file");
    }
    file.reset(ifstream.release());
    readChar();
  }

  void openFile(const std::string &filename) {
    openFile(filename.c_str());
  }

  /// Load a string using istringstream.
  void loadBuffer(const std::string &buffer) {
    file = std::make_unique<std::istringstream>(buffer);
    readChar();
  }

  /// Tokenise the input only and report the tokens.
  void emitTokens(std::ostream &out) {
    while (true) {
      switch (getNextToken()) {
        case Token::IDENTIFIER:
          out << "IDENTIFIER " << getIdentifier() << "\n";
          break;
        case Token::NUMBER:
          out << "NUMBER " << getNumber() << "\n";
          break;
        case Token::STRING:
          out << "STRING " << getString() << "\n";
          break;
        case Token::END_OF_FILE:
          out << "EOF\n";
          return;
        default:
          out << tokenEnumStr(getLastToken()) << "\n";
          break;
      }
    }
  }

  const std::string &getIdentifier() const { return identifier; }
  int getNumber() const { return value; }
  const std::string &getString() const { return string; }
  Token getLastToken() const { return lastToken; }
  size_t getLineNumber() const { return currentLineNumber; }
  size_t getCharNumber() const { return currentCharNumber; }
  bool hasLine() const { return !currentLine.empty(); }
  const std::string &getLine() const { return currentLine; }
  const Location getLocation() const { return Location(currentLineNumber,
                                                       currentCharNumber); }
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
class BinaryOpExpr;
class UnaryOpExpr;
class StringExpr;
class BooleanExpr;
class NumberExpr;
class CallExpr;
class ArraySubscriptExpr;
class VarRefExpr;
class ValFormal;
class ArrayFormal;
class ProcFormal;
class FuncFormal;
class SkipStatement;
class StopStatement;
class ReturnStatement;
class IfStatement;
class WhileStatement;
class SeqStatement;
class CallStatement;
class AssStatement;

/// A visitor base class for the AST.
struct AstVisitor {
  virtual void visitPre(Program&) {}
  virtual void visitPost(Program&) {}
  virtual void visitPre(Proc&) {}
  virtual void visitPost(Proc&) {}
  virtual void visitPre(ArrayDecl&) {}
  virtual void visitPost(ArrayDecl&) {}
  virtual void visitPre(VarDecl&) {}
  virtual void visitPost(VarDecl&) {}
  virtual void visitPre(ValDecl&) {}
  virtual void visitPost(ValDecl&) {}
  virtual void visitPre(BinaryOpExpr&) {}
  virtual void visitPost(BinaryOpExpr&) {}
  virtual void visitPre(UnaryOpExpr&) {}
  virtual void visitPost(UnaryOpExpr&) {}
  virtual void visitPre(StringExpr&) {}
  virtual void visitPost(StringExpr&) {}
  virtual void visitPre(BooleanExpr&) {}
  virtual void visitPost(BooleanExpr&) {}
  virtual void visitPre(NumberExpr&) {}
  virtual void visitPost(NumberExpr&) {}
  virtual void visitPre(CallExpr&) {}
  virtual void visitPost(CallExpr&) {}
  virtual void visitPre(ArraySubscriptExpr&) {}
  virtual void visitPost(ArraySubscriptExpr&) {}
  virtual void visitPre(VarRefExpr&) {}
  virtual void visitPost(VarRefExpr&) {}
  virtual void visitPre(ValFormal&) {}
  virtual void visitPost(ValFormal&) {}
  virtual void visitPre(ArrayFormal&) {}
  virtual void visitPost(ArrayFormal&) {}
  virtual void visitPre(ProcFormal&) {}
  virtual void visitPost(ProcFormal&) {}
  virtual void visitPre(FuncFormal&) {}
  virtual void visitPost(FuncFormal&) {}
  virtual void visitPre(SkipStatement&) {}
  virtual void visitPost(SkipStatement&) {}
  virtual void visitPre(StopStatement&) {}
  virtual void visitPost(StopStatement&) {}
  virtual void visitPre(ReturnStatement&) {}
  virtual void visitPost(ReturnStatement&) {}
  virtual void visitPre(IfStatement&) {}
  virtual void visitPost(IfStatement&) {}
  virtual void visitPre(WhileStatement&) {}
  virtual void visitPost(WhileStatement&) {}
  virtual void visitPre(SeqStatement&) {}
  virtual void visitPost(SeqStatement&) {}
  virtual void visitPre(CallStatement&) {}
  virtual void visitPost(CallStatement&) {}
  virtual void visitPre(AssStatement&) {}
  virtual void visitPost(AssStatement&) {}
};

/// AST node base class.
class AstNode {
  Location location;
public:
  AstNode() : location(Location(0,0)) {}
  AstNode(Location location) : location(location) {}
  virtual ~AstNode() = default;
  virtual void accept(AstVisitor* visitor) = 0;
  const Location &getLocation() const { return location; }
};

// Expressions ============================================================= //

class Expr : public AstNode {
  std::optional<int> constValue;
public:
  Expr(Location location) : AstNode(location), constValue(std::nullopt) {}
  bool isConst() const { return constValue.has_value(); }
  int getValue() const { return constValue.value(); }
  void setValue(int newConstValue) { constValue.emplace(newConstValue); }
};

class VarRefExpr : public Expr {
  std::string name;
  std::unique_ptr<Expr> expr;
public:
  VarRefExpr(Location location, std::string name) : Expr(location), name(name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
  const std::string &getName() const { return name; }
};

class ArraySubscriptExpr : public Expr {
  std::string name;
  std::unique_ptr<Expr> expr;
public:
  ArraySubscriptExpr(Location location, std::string name, std::unique_ptr<Expr> expr) :
      Expr(location), name(name), expr(std::move(expr)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    expr->accept(visitor);
    visitor->visitPost(*this);
  }
  const std::string &getName() const { return name; }
};

class CallExpr : public Expr {
  std::string name;
  std::vector<std::unique_ptr<Expr>> args;
public:
  CallExpr(Location location, std::string name) :
      Expr(location), name(name) {}
  CallExpr(Location location, std::string name, std::vector<std::unique_ptr<Expr>> args) :
      Expr(location), name(name), args(std::move(args)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    for (auto &arg : args) {
      arg->accept(visitor);
    }
    visitor->visitPost(*this);
  }
  const std::string &getName() const { return name; }
  const std::vector<std::unique_ptr<Expr>> &getArgs() const { return args; }
};

class NumberExpr : public Expr {
  unsigned value;
public:
  NumberExpr(Location location, unsigned value) :
      Expr(location), value(value) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
  unsigned getValue() const { return value; }
};

class BooleanExpr : public Expr {
  bool value;
public:
  BooleanExpr(Location location, bool value) :
      Expr(location), value(value) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
  bool getValue() const { return value; }
};

class StringExpr : public Expr {
  std::string value;
public:
  StringExpr(Location location, std::string value) :
      Expr(location), value(value) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
  std::string getValue() const { return value; }
};

class UnaryOpExpr : public Expr {
  Token op;
  std::unique_ptr<Expr> element;
public:
  UnaryOpExpr(Location location, Token op, std::unique_ptr<Expr> element) :
      Expr(location), op(op), element(std::move(element)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    if (!isConst()) {
      element->accept(visitor);
    }
    visitor->visitPost(*this);
  }
  Token getOp() const { return op; }
  Expr *getElement() const { return element.get(); }
};

class BinaryOpExpr : public Expr {
  Token op;
  std::unique_ptr<Expr> LHS, RHS;
public:
  BinaryOpExpr(Location location, Token op, std::unique_ptr<Expr> LHS, std::unique_ptr<Expr> RHS) :
      Expr(location), op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    if (!isConst()) {
      LHS->accept(visitor);
      RHS->accept(visitor);
    }
    visitor->visitPost(*this);
  }
  Token getOp() const { return op; }
  Expr *getLHS() const { return LHS.get(); }
  Expr *getRHS() const { return RHS.get(); }
};

// Declarations ============================================================ //

class Decl : public AstNode {
  std::string name;
public:
  Decl(Location location, std::string name) : AstNode(location), name(name) {}
  std::string getName() const { return name; }
};

class ValDecl : public Decl {
  std::unique_ptr<Expr> expr;
  int exprValue;
public:
  ValDecl(Location location, std::string name, std::unique_ptr<Expr> expr) :
      Decl(location, name), expr(std::move(expr)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    expr->accept(visitor);
    visitor->visitPost(*this);
  }
  Expr *getExpr() const { return expr.get(); }
  int getValue() const { return exprValue; }
  void setValue(int value) { exprValue = value; }
};

class VarDecl : public Decl {
public:
  VarDecl(Location location, std::string name) : Decl(location, name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

class ArrayDecl : public Decl {
  std::unique_ptr<Expr> expr;
public:
  ArrayDecl(Location location, std::string name, std::unique_ptr<Expr> expr) :
      Decl(location, name), expr(std::move(expr)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    expr->accept(visitor);
    visitor->visitPost(*this);
  }
};

// Formals ================================================================= //

class Formal : public AstNode {
  std::string name;
public:
  Formal(Location location, std::string name) : AstNode(location), name(name) {}
  std::string getName() const { return name; }
};

class ValFormal : public Formal {
public:
  ValFormal(Location location, std::string name) : Formal(location, name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

class ArrayFormal : public Formal {
public:
  ArrayFormal(Location location, std::string name) : Formal(location, name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

class ProcFormal : public Formal {
public:
  ProcFormal(Location location, std::string name) : Formal(location, name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

class FuncFormal : public Formal {
public:
  FuncFormal(Location location, std::string name) : Formal(location, name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

// Statement ================================================================ //

struct Statement : public AstNode {
  Statement(Location location) : AstNode(location) {}
};

class SkipStatement : public Statement {
public:
  SkipStatement(Location location) : Statement(location) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

class StopStatement : public Statement {
public:
  StopStatement(Location location) : Statement(location) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

class ReturnStatement : public Statement {
  std::unique_ptr<Expr> expr;
public:
  ReturnStatement(Location location, std::unique_ptr<Expr> expr) :
      Statement(location), expr(std::move(expr)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    expr->accept(visitor);
    visitor->visitPost(*this);
  }
};

class IfStatement : public Statement {
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Statement> thenStmt;
  std::unique_ptr<Statement> elseStmt;
public:
  IfStatement(Location location,
              std::unique_ptr<Expr> condition,
              std::unique_ptr<Statement> thenStmt,
              std::unique_ptr<Statement> elseStmt) :
      Statement(location),
      condition(std::move(condition)),
      thenStmt(std::move(thenStmt)),
      elseStmt(std::move(elseStmt)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    condition->accept(visitor);
    thenStmt->accept(visitor);
    elseStmt->accept(visitor);
    visitor->visitPost(*this);
  }
};

class WhileStatement : public Statement {
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Statement> stmt;
public:
  WhileStatement(Location location, 
                 std::unique_ptr<Expr> condition,
                 std::unique_ptr<Statement> stmt) :
      Statement(location),
      condition(std::move(condition)),
      stmt(std::move(stmt)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    condition->accept(visitor);
    stmt->accept(visitor);
    visitor->visitPost(*this);
  }
};

class SeqStatement : public Statement {
  std::vector<std::unique_ptr<Statement>> stmts;
public:
  SeqStatement(Location location, std::vector<std::unique_ptr<Statement>> stmts) :
      Statement(location), stmts(std::move(stmts)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    for (auto &stmt : stmts) {
      stmt->accept(visitor);
    }
    visitor->visitPost(*this);
  }
};

class CallStatement : public Statement {
  std::unique_ptr<CallExpr> call;
public:
  CallStatement(Location location, std::unique_ptr<CallExpr> call) :
      Statement(location), call(std::move(call)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    for (auto &arg : call.get()->getArgs()) {
      arg->accept(visitor);
    }
    visitor->visitPost(*this);
  }
  const std::string &getName() const { return call.get()->getName(); }
  const std::vector<std::unique_ptr<Expr>> &getArgs() { return call->getArgs(); }
};

class AssStatement : public Statement {
  std::unique_ptr<Expr> LHS, RHS;
public:
  AssStatement(Location location,
               std::unique_ptr<Expr> LHS,
               std::unique_ptr<Expr> RHS) :
      Statement(location), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    LHS->accept(visitor);
    RHS->accept(visitor);
    visitor->visitPost(*this);
  }
};

// Procedures and functions ================================================= //

class Proc : public AstNode {
  bool function;
  std::string name;
  std::vector<std::unique_ptr<Formal>> formals;
  std::vector<std::unique_ptr<Decl>> decls;
  std::unique_ptr<Statement> statement;
public:
  Proc(Location location,
       bool isFunction,
       std::string name,
       std::vector<std::unique_ptr<Formal>> formals,
       std::vector<std::unique_ptr<Decl>> decls,
       std::unique_ptr<Statement> statement) :
      AstNode(location), function(isFunction), name(name),
      formals(std::move(formals)), decls(std::move(decls)),
      statement(std::move(statement)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    for (auto &formal : formals) {
      formal->accept(visitor);
    }
    for (auto &decl : decls) {
      decl->accept(visitor);
    }
    statement->accept(visitor);
    visitor->visitPost(*this);
  }
  bool isFunction() const { return function; }
  const std::string &getName() const { return name; }
  std::vector<std::unique_ptr<Formal>> &getFormals() { return formals; }
  std::vector<std::unique_ptr<Decl>> &getDecls() { return decls; }
  Statement *getStatement() { return statement.get(); }
};

class Program : public AstNode {
  std::vector<std::unique_ptr<Decl>> globalDecls;
  std::vector<std::unique_ptr<Proc>> procDecls;
public:
  Program(std::vector<std::unique_ptr<Decl>> globals,
          std::vector<std::unique_ptr<Proc>> procs) :
      globalDecls(std::move(globals)), procDecls(std::move(procs)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    for (auto &decl : globalDecls) {
      decl->accept(visitor);
    }
    for (auto &proc : procDecls) {
      proc->accept(visitor);
    }
    visitor->visitPost(*this);
  }
};

// AST printer visitor ===================================================== //

class AstPrinter : public AstVisitor {
  std::ostream outs;
  unsigned indentCount;
  void indent() {
    for (size_t i=0; i<indentCount; i++) {
      outs << "  ";
    }
  }
  std::string exprValString(const Expr &expr) {
    return expr.isConst() ? (boost::format(" [const=%d]") % expr.getValue()).str() : "";
  }
  std::string locString(const AstNode &node) {
    return (boost::format(" [loc=%s]") % node.getLocation().str()).str();
  }
public:
  AstPrinter(std::ostream& outs = std::cout) :
      outs(outs.rdbuf()), indentCount(0) {}
  void visitPre(Program &decl) override {
    indent(); outs << "program\n";
    indentCount++;
  };
  void visitPost(Program &decl) override {
    indentCount--;
  }
  void visitPre(Proc &decl) override {
    indent(); outs << boost::format("proc %s%s\n") % decl.getName() % locString(decl);
    indentCount++;
  };
  void visitPost(Proc &decl) override {
    indentCount--;
  }
  void visitPre(ArrayDecl &decl) override {
    indent(); outs << boost::format("arraydecl %s%s\n") % decl.getName() % locString(decl);
    indentCount++;
  };
  void visitPost(ArrayDecl &decl) override {
    indentCount--;
  }
  void visitPre(VarDecl &decl) override {
    indent(); outs << boost::format("vardecl %s%s\n") % decl.getName() % locString(decl);
  };
  void visitPost(VarDecl &decl) override { }
  void visitPre(ValDecl &decl) override {
    indent(); outs << boost::format("valdecl %s%s\n") % decl.getName() % locString(decl);
    indentCount++;
  };
  void visitPost(ValDecl &decl) override {
    indentCount--;
  }
  void visitPre(BinaryOpExpr &expr) override {
    indent(); outs << boost::format("binaryop %s%s%s\n")
                        % tokenEnumStr(expr.getOp()) % exprValString(expr) % locString(expr);
    indentCount++;
  };
  void visitPost(BinaryOpExpr &expr) override {
    indentCount--;
  }
  void visitPre(UnaryOpExpr &expr) override {
    indent(); outs << boost::format("unaryop %s%s%s\n")
                        % tokenEnumStr(expr.getOp()) % exprValString(expr) % locString(expr);
    indentCount++;
  };
  void visitPost(UnaryOpExpr &expr) override {
    indentCount--;
  }
  void visitPre(StringExpr &expr) override {
    indent(); outs << boost::format("string %s%s\n") % expr.getValue() % locString(expr);
  };
  void visitPost(StringExpr &expr) override { }
  void visitPre(BooleanExpr &expr) override {
    indent(); outs << boost::format("boolean %d%s\n") % expr.getValue() % locString(expr);
  };
  void visitPost(BooleanExpr &expr) override { }
  void visitPre(NumberExpr &expr) override {
    indent(); outs << boost::format("number %d%s\n") % expr.getValue() % locString(expr);
  };
  void visitPost(NumberExpr &expr) override { }
  void visitPre(CallExpr &expr) override {
    indent(); outs << boost::format("call %s%s\n") % expr.getName() % locString(expr);
    indentCount++;
  };
  void visitPost(CallExpr &expr) override {
    indentCount--;
  }
  void visitPre(ArraySubscriptExpr &expr) override {
    indent(); outs << boost::format("arraysubscript %s%s\n") % expr.getName() % locString(expr);
    indentCount++;
  };
  void visitPost(ArraySubscriptExpr &expr) override {
    indentCount--;
  }
  void visitPre(VarRefExpr &expr) override {
    indent(); outs << boost::format("varref %s%s\n") % expr.getName() % locString(expr);
  };
  void visitPost(VarRefExpr &expr) override { }
  void visitPre(ValFormal &formal) override {
    indent(); outs << boost::format("valformal %s%s\n") % formal.getName() % locString(formal);
  };
  void visitPost(ValFormal &formal) override {};
  void visitPre(ArrayFormal &formal) override {
    indent(); outs << boost::format("arrayformal %s%s\n") % formal.getName() % locString(formal);
  };
  void visitPost(ArrayFormal &formal) override {};
  void visitPre(ProcFormal &formal) override {
    indent(); outs << boost::format("procformal %s%s\n") % formal.getName() % locString(formal);
  };
  void visitPost(ProcFormal &formal) override {};
  void visitPre(FuncFormal &formal) override {
    indent(); outs << boost::format("funcformal %s%s\n") % formal.getName() % locString(formal);
  };
  void visitPost(FuncFormal &formal) override {};
  void visitPre(SkipStatement &stmt) override {
    indent(); outs << boost::format("skipstmt%s\n") % locString(stmt);
  };
  void visitPost(SkipStatement &stmt) override {};
  void visitPre(StopStatement &stmt) override {
    indent(); outs << boost::format("stopstmt%s\n") % locString(stmt);
  };
  void visitPost(StopStatement &stmt) override {};
  void visitPre(ReturnStatement &stmt) override {
    indent(); outs << boost::format("returnstmt%s\n") % locString(stmt);
    indentCount++;
  };
  void visitPost(ReturnStatement &stmt) override {
    indentCount--;
  };
  void visitPre(IfStatement &stmt) override {
    indent(); outs << boost::format("ifstmt%s\n") % locString(stmt);
    indentCount++;
  };
  void visitPost(IfStatement &stmt) override {
    indentCount--;
  };
  void visitPre(WhileStatement &stmt) override {
    indent(); outs << boost::format("whilestmt%s\n") % locString(stmt);
    indentCount++;
  };
  void visitPost(WhileStatement &stmt) override {
    indentCount--;
  };
  void visitPre(SeqStatement &stmt) override {
    indent(); outs << boost::format("seqstmt%s\n") % locString(stmt);
    indentCount++;
  };
  void visitPost(SeqStatement &stmt) override {
    indentCount--;
  };
  void visitPre(CallStatement &stmt) override {
    indent(); outs << boost::format("callstmt %s%s\n") % stmt.getName() % locString(stmt);
    indentCount++;
  };
  void visitPost(CallStatement &stmt) override {
    indentCount--;
  };
  void visitPre(AssStatement &stmt) override {
    indent(); outs << boost::format("assstmt%s\n") % locString(stmt);
    indentCount++;
  };
  void visitPost(AssStatement &stmt) override {
    indentCount--;
  };
};

//===---------------------------------------------------------------------===//
// Parser
//===---------------------------------------------------------------------===//

class Parser {
  Lexer &lexer;

  /// Expect the given last token, otherwise raise an error.
  void expect(Token token) const {
    if (token != lexer.getLastToken()) {
      throw UnexpectedTokenError(lexer.getLocation(), token, lexer.getLastToken());
    }
    lexer.getNextToken();
  }

  /// identifier
  std::string parseIdentifier() {
    if (lexer.getLastToken() == Token::IDENTIFIER) {
      auto name = lexer.getIdentifier();
      lexer.getNextToken();
      return name;
    } else {
      throw ExpectedNameError(lexer.getLocation(), lexer.getLastToken());
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
    auto location = lexer.getLocation();
    auto element = parseElement();
    if (isAssociative(op) && op == lexer.getLastToken()) {
      lexer.getNextToken();
      auto RHS = parseBinOpRHS(op);
      return std::make_unique<BinaryOpExpr>(location, op, std::move(element), std::move(RHS));
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
    auto location = lexer.getLocation();
    // Unary operations.
    if (lexer.getLastToken() == Token::MINUS) {
      auto location = lexer.getLocation();
      lexer.getNextToken();
      auto element = parseElement();
      return std::make_unique<UnaryOpExpr>(location, Token::MINUS, std::move(element));
    }
    if (lexer.getLastToken() == Token::NOT) {
      lexer.getNextToken();
      auto element = parseElement();
      return std::make_unique<UnaryOpExpr>(location, Token::NOT, std::move(element));
    }
    auto element = parseElement();
    auto op = lexer.getLastToken();
    if (isBinaryOp(op)) {
      // Binary operation.
      lexer.getNextToken();
      auto RHS = parseBinOpRHS(op);
      return std::make_unique<BinaryOpExpr>(location, op, std::move(element), std::move(RHS));
    }
    // Otherwise just return an element.
    return element;
  }

  /// expression-list :=
  ///   <expr> [ "," <expr> ]
  std::vector<std::unique_ptr<Expr>> parseExprList() {
    std::vector<std::unique_ptr<Expr>> exprList;
    exprList.push_back(parseExpr());
    while (lexer.getLastToken() == Token::COMMA) {
      lexer.getNextToken();
      exprList.push_back(parseExpr());
    }
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
  ///   "(" ")"
  ///   "(" <expr> ")"
  std::unique_ptr<Expr> parseElement() {
    auto location = lexer.getLocation();
    switch (lexer.getLastToken()) {
    case Token::IDENTIFIER: {
      auto name = parseIdentifier();
      // Array subscript.
      if (lexer.getLastToken() == Token::LBRACKET) {
        lexer.getNextToken();
        auto expr = parseExpr();
        expect(Token::RBRACKET);
        return std::make_unique<ArraySubscriptExpr>(location, name, std::move(expr));
      // Procedure call.
      } else if (lexer.getLastToken() == Token::LPAREN) {
        if (lexer.getNextToken() == Token::RPAREN) {
          lexer.getNextToken();
          return std::make_unique<CallExpr>(location, name);
        } else {
          auto exprList = parseExprList();
          expect(Token::RPAREN);
          return std::make_unique<CallExpr>(location, name, std::move(exprList));
        }
      // Variable reference.
      } else {
        return std::make_unique<VarRefExpr>(location, name);
      }
    }
    case Token::NUMBER:
      lexer.getNextToken();
      return std::make_unique<NumberExpr>(location, lexer.getNumber());
    case Token::STRING:
      lexer.getNextToken();
      return std::make_unique<StringExpr>(location, lexer.getString());
    case Token::TRUE:
      lexer.getNextToken();
      return std::make_unique<BooleanExpr>(location, true);
    case Token::FALSE:
      lexer.getNextToken();
      return std::make_unique<BooleanExpr>(location, false);
    case Token::LPAREN: {
      lexer.getNextToken();
      auto expr = parseExpr();
      expect(Token::RPAREN);
      return expr;
    }
    default:
      throw ParserTokenError(location, "in expression element", lexer.getLastToken());
    }
  }

  /// declaration :=
  ///   "val" <identifier> "=" <expr> ";"
  ///   "var" <identifier> ";"
  ///   "array" <identifier> "[" <expr> "]" ";"
  std::unique_ptr<Decl> parseDecl() {
    auto location = lexer.getLocation();
    switch (lexer.getLastToken()) {
    case Token::VAL: {
      lexer.getNextToken();
      auto name = parseIdentifier();
      expect(Token::EQ);
      auto expr = parseExpr();
      expect(Token::SEMICOLON);
      return std::make_unique<ValDecl>(location, name, std::move(expr));
    }
    case Token::VAR: {
      lexer.getNextToken();
      auto name = parseIdentifier();
      expect(Token::SEMICOLON);
      return std::make_unique<VarDecl>(location, name);
    }
    case Token::ARRAY: {
      lexer.getNextToken();
      auto name = parseIdentifier();
      expect(Token::LBRACKET);
      auto expr = parseExpr();
      expect(Token::RBRACKET);
      expect(Token::SEMICOLON);
      return std::make_unique<ArrayDecl>(location, name, std::move(expr));
    }
    default:
      throw ParserTokenError(location, "invalid declaration", lexer.getLastToken());
    }
  }

  /// local-decls :=
  ///   [0 <local-decl> ]
  /// local-decl :=
  ///   "val" ...
  ///   "var" ...
  std::vector<std::unique_ptr<Decl>> parseLocalDecls() {
    std::vector<std::unique_ptr<Decl>> decls;
    while (lexer.getLastToken() == Token::VAL ||
           lexer.getLastToken() == Token::VAR) {
      decls.push_back(parseDecl());
    }
    return decls;
  }

  /// global-decls :=
  ///   [0 <global-decl> ]
  /// global-decl :=
  ///   "val" ...
  ///   "var" ...
  ///   "global" ...
  std::vector<std::unique_ptr<Decl>> parseGlobalDecls() {
    std::vector<std::unique_ptr<Decl>> decls;
    while (lexer.getLastToken() == Token::VAL ||
           lexer.getLastToken() == Token::VAR ||
           lexer.getLastToken() == Token::ARRAY) {
      decls.push_back(parseDecl());
    }
    return decls;
  }

  /// formals :=
  ///   [0 <formal> "," ]
  std::vector<std::unique_ptr<Formal>> parseFormals() {
    std::vector<std::unique_ptr<Formal>> formals;
      while (true) {
      formals.push_back(parseFormal());
      if (lexer.getLastToken() == Token::COMMA) {
        lexer.getNextToken();
      } else {
        break;
      }
    }
    return formals;
  }

  /// formal :=
  ///   "val" <name>
  ///   "array" <name>
  ///   "proc" <name>
  ///   "func" <name>
  std::unique_ptr<Formal> parseFormal() {
    auto location = lexer.getLocation();
    switch (lexer.getLastToken()) {
    case Token::VAL:
      lexer.getNextToken();
      return std::make_unique<ValFormal>(location, parseIdentifier());
    case Token::ARRAY:
      lexer.getNextToken();
      return std::make_unique<ArrayFormal>(location, parseIdentifier());
    case Token::PROC:
      lexer.getNextToken();
      return std::make_unique<ProcFormal>(location, parseIdentifier());
    case Token::FUNC:
      lexer.getNextToken();
      return std::make_unique<FuncFormal>(location, parseIdentifier());
    default:
      throw ParserTokenError(location, "invalid formal", lexer.getLastToken());
    }
  }

  /// statements :=
  ///   [1 <stmt> "," ]
  std::vector<std::unique_ptr<Statement>> parseStatements() {
    std::vector<std::unique_ptr<Statement>> stmts;
    stmts.push_back(parseStatement());
    while (lexer.getLastToken() == Token::SEMICOLON) {
      lexer.getNextToken();
      stmts.push_back(parseStatement());
    }
    return stmts;
  }

  /// statement :=
  ///   "skip"
  ///   "stop"
  ///   "return" <expr>
  ///   "if" <expr> "then" <stmt> "else" <stmt>
  ///   "while" <expr> "do" <stmt>
  ///   "{" [1 <stmt> "," ] "}"
  ///   <identifier> ":=" <expr>
  ///   <identifier> "(" [ <expr> "," ] ")"
  std::unique_ptr<Statement> parseStatement() {
    auto location = lexer.getLocation();
    switch (lexer.getLastToken()) {
    case Token::SKIP:
      lexer.getNextToken();
      return std::make_unique<SkipStatement>(location);
    case Token::STOP:
      lexer.getNextToken();
      return std::make_unique<StopStatement>(location);
    case Token::RETURN:
      lexer.getNextToken();
      return std::make_unique<ReturnStatement>(location, parseExpr());
    case Token::IF: {
      lexer.getNextToken();
      auto condition = parseExpr();
      expect(Token::THEN);
      auto thenStmt = parseStatement();
      expect(Token::ELSE);
      auto elseStmt = parseStatement();
      return std::make_unique<IfStatement>(location,
                                           std::move(condition),
                                           std::move(thenStmt),
                                           std::move(elseStmt));
    }
    case Token::WHILE: {
      lexer.getNextToken();
      auto condition = parseExpr();
      expect(Token::DO);
      auto stmt = parseStatement();
      return std::make_unique<WhileStatement>(location,
                                              std::move(condition),
                                              std::move(stmt));
    }
    case Token::BEGIN: {
      lexer.getNextToken();
      auto body = parseStatements();
      expect(Token::END);
      return std::make_unique<SeqStatement>(location, std::move(body));
    }
    case Token::IDENTIFIER: {
      auto element = parseElement();
      // Procedure call
      if (dynamic_cast<CallExpr*>(element.get())) {
        auto callExpr = std::unique_ptr<CallExpr>(static_cast<CallExpr*>(element.release()));
        return std::make_unique<CallStatement>(location, std::move(callExpr));
      }
      // Assignment
      expect(Token::ASS);
      return std::make_unique<AssStatement>(location, std::move(element), parseExpr());
    }
    default:
      throw ParserTokenError(location, "invalid statement", lexer.getLastToken());
    }
  }

  /// proc-decl :=
  ///  "proc" <name> "(" <formals> ")" "is" [0 <var-decl> ] <statement>
  std::unique_ptr<Proc> parseProcDecl() {
    auto location = lexer.getLocation();
    bool isFunction = lexer.getLastToken() == Token::FUNC;
    lexer.getNextToken();
    // Name
    auto name = parseIdentifier();
    // Formals
    expect(Token::LPAREN);
    std::vector<std::unique_ptr<Formal>> formals;
    if (lexer.getLastToken() == Token::RPAREN) {
      lexer.getNextToken();
    } else {
      formals = parseFormals();
      expect(Token::RPAREN);
    }
    // "is"
    expect(Token::IS);
    // Declarations
    std::vector<std::unique_ptr<Decl>> decls;
    if (lexer.getLastToken() == Token::VAL ||
        lexer.getLastToken() == Token::VAR) {
      decls = parseLocalDecls();
    }
    auto statement = parseStatement();
    return std::make_unique<Proc>(location, isFunction, name, std::move(formals),
                                  std::move(decls), std::move(statement));
  }

  /// proc-decls :=
  ///   [1 <proc-decl> ]
  std::vector<std::unique_ptr<Proc>> parseProcDecls() {
    std::vector<std::unique_ptr<Proc>> procDecls;
    while (lexer.getLastToken() == Token::PROC ||
           lexer.getLastToken() == Token::FUNC) {
      procDecls.push_back(parseProcDecl());
    }
    return procDecls;
  }

public:
  Parser(Lexer &lexer) : lexer(lexer) {}

  std::unique_ptr<Program> parseProgram() {
    lexer.getNextToken();
    auto globalDecls = parseGlobalDecls();
    auto procDecls = parseProcDecls();
    lexer.getNextToken();
    expect(Token::END_OF_FILE);
    return std::make_unique<Program>(std::move(globalDecls),
                                     std::move(procDecls));
  }
};

//===---------------------------------------------------------------------===//
// Symbol table.
//===---------------------------------------------------------------------===//

enum class SymbolType {
  VAL,
  VAR,
  ARRAY,
  FUNC,
  PROC
};

enum class SymbolScope {
  GLOBAL,
  LOCAL
};

class Frame {
  size_t size; // The size of the frame.
  size_t offset;   // The current stack pointer offset value.
public:
  Frame() : size(0) {}
  void setSize(int newSize) { size = newSize > size ? newSize : size; }
  int getSize() { return size; }
  void incOffset(int amount) { offset += amount; }
  size_t getOffset() { return offset; }
};

/// A class to represent a symbol in the program, recording type, scope, AST
/// declaration and Frame infromation where applicable.
class Symbol {
  SymbolType type;
  SymbolScope scope;
  AstNode *node;
  std::string name;
  std::unique_ptr<Frame> frame;
  int stackOffset;
  std::string globalLabel;

public:
  Symbol(SymbolType type, SymbolScope scope, AstNode *node, const std::string &name) :
      type(type), scope(scope), node(node), name(name) {}
  SymbolType getType() const { return type; }
  SymbolScope getScope() const { return scope; }
  AstNode *getNode() const { return node; }
  const std::string &getName() const { return name; }
  void setFrame(std::unique_ptr<Frame> newFrame) { frame = std::move(newFrame); }
  int getStackOffset() const { return stackOffset; }
  void setStackOffset(int value) { stackOffset = value; }
  Frame *getFramePtr() { return frame.get(); }
  const std::string &getGlobalLabel() const { return globalLabel; }
  void setGlobalLabel(const std::string &value) { globalLabel = value; }
};

class SymbolTable {
  std::map<const std::string, std::unique_ptr<Symbol>> symbolMap;

public:
  void insert(const std::string &name, std::unique_ptr<Symbol> symbol) {
    symbolMap[name] = std::move(symbol);
  }

  /// Lookup a symbol, and throw an exception if not found.
  Symbol* lookup(const std::string &name, const Location &location) {
    auto it = symbolMap.find(name);
    if (it != symbolMap.end()) {
      return it->second.get();
    }
    throw UnknownSymbolError(location, name);
  }
};

//===---------------------------------------------------------------------===//
// Constant propagation.
//===---------------------------------------------------------------------===//

class ConstProp : public AstVisitor {
  SymbolTable &symbolTable;
  std::stack<SymbolScope> scope;
public:
  ConstProp(SymbolTable &symbolTable) : symbolTable(symbolTable) {}
  virtual void visitPre(Program&) {
    scope.push(SymbolScope::GLOBAL);
  }
  virtual void visitPost(Program&) {
    scope.pop();
  }
  void visitPre(Proc &proc) {
    scope.push(SymbolScope::LOCAL);
    symbolTable.insert(proc.getName(), std::make_unique<Symbol>(SymbolType::PROC, scope.top(), &proc, proc.getName()));
  }
  void visitPost(Proc &proc) {
    scope.pop();
  }
  void visitPre(ArrayDecl &decl) {
    symbolTable.insert(decl.getName(), std::make_unique<Symbol>(SymbolType::ARRAY, scope.top(), &decl, decl.getName()));
  }
  void visitPre(VarDecl &decl) {
    symbolTable.insert(decl.getName(), std::make_unique<Symbol>(SymbolType::VAR, scope.top(), &decl, decl.getName()));
  }
  void visitPre(ValDecl &decl) {
    symbolTable.insert(decl.getName(), std::make_unique<Symbol>(SymbolType::VAL, scope.top(), &decl, decl.getName()));
  }
  void visitPost(ValDecl &decl) {
    if (decl.getExpr()->isConst()) {
      decl.setValue(decl.getExpr()->getValue());
    }
  }
  void visitPre(ValFormal &formal) {
    symbolTable.insert(formal.getName(), std::make_unique<Symbol>(SymbolType::VAL, scope.top(), &formal, formal.getName()));
  }
  void visitPre(ArrayFormal &formal) {
    symbolTable.insert(formal.getName(), std::make_unique<Symbol>(SymbolType::ARRAY, scope.top(), &formal, formal.getName()));
  }
  void visitPre(ProcFormal &formal) {
    symbolTable.insert(formal.getName(), std::make_unique<Symbol>(SymbolType::PROC, scope.top(), &formal, formal.getName()));
  }
  void visitPre(FuncFormal &formal) {
    symbolTable.insert(formal.getName(), std::make_unique<Symbol>(SymbolType::FUNC, scope.top(), &formal, formal.getName()));
  }
  void visitPost(BinaryOpExpr &expr) {
    auto LHS = expr.getLHS();
    auto RHS = expr.getRHS();
    if (LHS->isConst() &&
        RHS->isConst()) {
      // Evaluate binary expression.
      int result;
      switch (expr.getOp()) {
        case Token::PLUS:  result = LHS->getValue() +  RHS->getValue(); break;
        case Token::MINUS: result = LHS->getValue() -  RHS->getValue(); break;
        case Token::OR:    result = LHS->getValue() |  RHS->getValue(); break;
        case Token::AND:   result = LHS->getValue() &  RHS->getValue(); break;
        case Token::EQ:    result = LHS->getValue() == RHS->getValue(); break;
        case Token::NE:    result = LHS->getValue() != RHS->getValue(); break;
        case Token::LS:    result = LHS->getValue() <  RHS->getValue(); break;
        case Token::LE:    result = LHS->getValue() <= RHS->getValue(); break;
        case Token::GR:    result = LHS->getValue() >  RHS->getValue(); break;
        case Token::GE:    result = LHS->getValue() >= RHS->getValue(); break;
        default:
          throw SemanticTokenError(expr.getLocation(), "unexpected binary op", expr.getOp());
      }
      expr.setValue(result);
    }
  }
  void visitPost(UnaryOpExpr &expr) {
    auto element = expr.getElement();
    if (element->isConst()) {
      // Evaluate unary expression.
      int result;
      switch (expr.getOp()) {
        case Token::MINUS: result = -element->getValue();
        case Token::NOT:   result = ~element->getValue();
        default:
          throw SemanticTokenError(expr.getLocation(), "unexpected unary op", expr.getOp());
      }
      expr.setValue(result);
    }
  }
  void visitPost(StringExpr &expr) {}
  void visitPost(BooleanExpr &expr) {
    expr.setValue(expr.getValue());
  }
  void visitPost(NumberExpr &expr) {
    expr.setValue(expr.getValue());
  }
  void visitPost(CallExpr &expr) {}
  void visitPost(ArraySubscriptExpr &expr) {}
  void visitPost(VarRefExpr &expr) {
    auto symbol = symbolTable.lookup(expr.getName(), expr.getLocation());
    if (auto valDecl = dynamic_cast<const ValDecl*>(symbol->getNode())) {
      expr.setValue(valDecl->getValue());
    }
  }
};

//===---------------------------------------------------------------------===//
// Code generation.
//===---------------------------------------------------------------------===//

const int SP_OFFSET = 1;
const int SP_INIT_VALUE = 1 << 16;

enum class Reg { A, B };

class IntermediateDirective : public hexasm::Directive {
public:
  IntermediateDirective(hexasm::Token token) : hexasm::Directive(token) {}
  bool operandIsLabel() const { return false; }
  size_t getSize() const { return 0; }
  int getValue() const { return 0; }
};

/// Procedure/function prologue.
class Prologue : public IntermediateDirective {
  Symbol *symbol;
public:
  Prologue(Symbol *symbol) : IntermediateDirective(hexasm::Token::PROLOGUE), symbol(symbol) {}
  std::string toString() const { return "PROLOGUE " + symbol->getName(); }
};

/// Procedure/function epilogue.
class Epilogue : public IntermediateDirective {
  Symbol *symbol;
public:
  Epilogue(Symbol *symbol) : IntermediateDirective(hexasm::Token::EPILOGUE), symbol(symbol) {}
  std::string toString() const { return "EPILOGUE " + symbol->getName(); }
};

/// Instruction relative offset to stack frame base.
class InstrStackOffset : public hexasm::InstrImm {
  Frame *frame;
  int offset;
public:
  InstrStackOffset(hexasm::Token token, Frame *frame, int offset) :
      InstrImm(token, 0), frame(frame), offset(offset) {}
  Frame *getFrame() { return frame; }
  int getOffset() { return offset; }
};

class CodeBuffer {
  SymbolTable &symbolTable;
  std::vector<std::unique_ptr<hexasm::Directive>> instrs;
  std::vector<std::unique_ptr<hexasm::Directive>> data;
  std::map<int, std::string> constMap;
  size_t constCount;
  size_t stringCount;
  size_t labelCount;
  Frame *currentFrame;


public:
  CodeBuffer(SymbolTable &symbolTable) : symbolTable(symbolTable), constCount(0), stringCount(0) {}

  const std::string getLabel() { return std::string("lab") + std::to_string(labelCount++); }

  /// Directive generation -------------------------------------------------///
  void genData(uint32_t value) { data.push_back(std::make_unique<hexasm::Data>(hexasm::Token::DATA, value)); }
  void genDataLabel(std::string name) { data.push_back(std::make_unique<hexasm::Label>(hexasm::Token::IDENTIFIER, name)); }
  void genLabel(std::string name) { instrs.push_back(std::make_unique<hexasm::Label>(hexasm::Token::IDENTIFIER, name)); }

  /// Instruction generation -----------------------------------------------///
  void genLDAM(int value)         { instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::LDAM, value)); }
  void genLDBM(int value)         { instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::LDBM, value)); }
  void genSTAM(int value)         { instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::STAM, value)); }
  void genLDAM(std::string label) { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::LDAM, label, false)); }
  void genLDBM(std::string label) { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::LDBM, label, false)); }
  void genSTAM(std::string label) { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::STAM, label, false)); }
  void genLDAC(int value)         { instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::LDAC, value)); }
  void genLDBC(int value)         { instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::LDBC, value)); }
  void genLDAP(int value)         { instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::LDAP, value)); }
  void genLDAC(std::string label) { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::LDAC, label, true)); }
  void genLDBC(std::string label) { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::LDBC, label, true)); }
  void genLDAP(std::string label) { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::LDAP, label, true)); }
  void genLDAI(int value)         { instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::LDAI, value)); }
  void genLDBI(int value)         { instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::LDBI, value)); }
  void genSTAI(int value)         { instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::STAI, value)); }
  void genBR(std::string label)   { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::BR, label, true)); }
  void genBRZ(std::string label)  { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::BRZ, label, true)); }
  void genBRN(std::string label)  { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::BRN, label, true)); }
  void genOPR(hexasm::Token op)   { instrs.push_back(std::make_unique<hexasm::InstrOp>(hexasm::Token::OPR, op)); }

  /// Procedure calling and frame-base relative accesses -------------------///
  void genPrologue(Symbol *symbol) { instrs.push_back(std::make_unique<Epilogue>(symbol)); }
  void genEpilogue(Symbol *symbol) { instrs.push_back(std::make_unique<Prologue>(symbol)); }
  void genLDAI_FB(Frame *frame, int offset) { instrs.push_back(std::make_unique<InstrStackOffset>(hexasm::Token::LDAI_FB, frame, offset)); }
  void genLDBI_FB(Frame *frame, int offset) { instrs.push_back(std::make_unique<InstrStackOffset>(hexasm::Token::LDBI_FB, frame, offset)); }
  void genSTAI_FB(Frame *frame, int offset) { instrs.push_back(std::make_unique<InstrStackOffset>(hexasm::Token::STAI_FB, frame, offset)); }

  /// Helpers --------------------------------------------------------------///
  void genLDSPB() { genLDBM(SP_OFFSET); } // Load SP into breg
  void genBRB() { genOPR(hexasm::Token::OPR); }
  void genADD() { genOPR(hexasm::Token::ADD); }
  void genSUB() { genOPR(hexasm::Token::SUB); }
  void genSVC() { genOPR(hexasm::Token::SVC); }

  /// Code generation visitors ---------------------------------------------///

  class ContainsCall : public AstVisitor {
    bool flag;
  public:
    ContainsCall() : flag(false) {}
    void visitPost(CallExpr&) { flag = true; }
    bool getFlag() { return flag; }
  };

  class ExprCodeGen : public AstVisitor {
    SymbolTable &st;
    CodeBuffer &cb;
  public:
    ExprCodeGen(SymbolTable &st, CodeBuffer &cb) : st(st), cb(cb) {}
    void visitPost(BinaryOpExpr &expr) {
      if (expr.isConst()) {
        cb.genConst(Reg::A, expr.getValue());
      } else {
        // generate binary op
        // generate bool for not, and, or, eq, ls
      }
    }
    void visitPost(UnaryOpExpr &expr) {
      if (expr.isConst()) {
        cb.genConst(Reg::A, expr.getValue());
      } else {
        // generate unary op
      }
    }
    void visitPost(StringExpr &expr) {
      cb.genString(Reg::A, expr.getValue());
    }
    void visitPost(BooleanExpr &expr) {
      cb.genConst(Reg::A, expr.getValue());
    }
    void visitPost(NumberExpr &expr) {
      cb.genConst(Reg::A, expr.getValue());
    }
    void visitPost(CallExpr &expr) {
      auto symbol = st.lookup(expr.getName(), expr.getLocation());
      cb.genCall(expr.getName(), expr.getArgs(), symbol->getType() == SymbolType::FUNC);
    }
    void visitPost(ArraySubscriptExpr &expr) {
      // generate array subscript
    }
    void visitPost(VarRefExpr &expr) {
      if (expr.isConst()) {
        cb.genConst(Reg::A, expr.getValue());
      } else {
        cb.genVar(Reg::A, st.lookup(expr.getName(), expr.getLocation()));
      }
    }
  };

  /// Code generation ------------------------------------------------------///

  void genExpr(Expr *expr) {
    ExprCodeGen visitor(symbolTable, *this);
    expr->accept(&visitor);
  }

  /// Generate a constant pool entry if required, return the label to it.
  const std::string genConstPool(int value) {
    if (constMap.count(value) == 0) {
      auto label = (boost::format("_const%d") % constCount++).str();
      constMap[value].assign(label);
      genDataLabel(label);
      genData(value);
      return label;
    } else {
      return constMap[value];
    }
  }

  /// Generate a constant value.
  void genConst(Reg reg, int value) {
    if (value > -(1<<16) && value < (1<<16)) {
      // Load 16-bit values from prefixed operands.
      switch (reg) {
      case Reg::A: genLDAC(value); break;
      case Reg::B: genLDBC(value); break;
      }
    } else {
      // Load larger values from the constant pool, adding them only if they don't exist.
      auto label = genConstPool(value);
      switch (reg) {
      case Reg::A: genLDAC(label); break;
      case Reg::B: genLDBC(label); break;
      }
    }
  }

  /// Generate an address to a string literal.
  void genString(Reg reg, const std::string value) {
    // Add string to pool and assign label.
    // Create the label.
    auto label = (boost::format("_string%d") % stringCount++).str();
    genDataLabel(label);
    // Back the string bytes into 32-bit words.
    uint32_t packedWord = 0;
    for (size_t strByteIndex = 0; strByteIndex < value.size(); strByteIndex++) {
      auto bytePos = strByteIndex % 4;
      packedWord |= value[strByteIndex] << bytePos;
      if (bytePos == 3 || strByteIndex == (value.size() - 1)) {
        genData(packedWord);
        packedWord = 0;
      }
    }
    // Load the address of the string.
    switch (reg) {
    case Reg::A: genLDAC(label); break;
    case Reg::B: genLDBC(label); break;
    }
  }

  /// Generate the value of a variable.
  void genVar(Reg reg, Symbol *symbol) {
    switch (symbol->getScope()) {
    // Load from stack.
    case SymbolScope::LOCAL: {
      switch (reg) {
      case Reg::A:
        genLDAM(SP_OFFSET);
        genLDAI(symbol->getStackOffset());
        break;
      case Reg::B:
        genLDBM(SP_OFFSET);
        genLDBI(symbol->getStackOffset());
        break;
      }
    }
    // Load from globals.
    case SymbolScope::GLOBAL: {
      switch (reg) {
      case Reg::A:
        genLDAM(symbol->getGlobalLabel());
        break;
      case Reg::B:
        genLDBM(symbol->getGlobalLabel());
        break;
      }
      break;
    }
    }
  }

  /// Return true if the expression contains a call.
  bool containsCall(Expr *expr) {
    ContainsCall visitor;
    expr->accept(&visitor);
    return visitor.getFlag();
  }

  /// Generate actual parameter to a call.
  /// Translate all expressions with calls
  ///
  void genActuals(const std::vector<std::unique_ptr<Expr>> &args) {
    // Generate actuals.
    for (auto &arg : args) {
      if (containsCall(arg.get())) {
        // For each actual expression containing one or more calls, allocate a stack word (FB relative) for the result of that call.
        genExpr(arg.get());
        genLDBM(SP_OFFSET);
        genSTAI_FB(currentFrame, currentFrame->getOffset());
        currentFrame->incOffset(1);
      } else {
        // For each actual not containing a call, load the the value into stack actual position.
      }
    }
  }

  /// Generate instructions to branch and link to a procedure.
  void genBranchAndLink(const std::string &name) {
    auto linkLabel = getLabel();
    genLDAP(linkLabel);
    genBR(name);
    genLabel(linkLabel);
  }

  /// Generate a procedure call.
  /// syscalls
  /// functions
  /// processes
  void genCall(const std::string &name, const std::vector<std::unique_ptr<Expr>> &args, bool isFunction) {
    size_t stackPointer = currentFrame->getSize();
    if (isFunction) {
      genActuals(args);
      genBranchAndLink(name);
      genLDAI(1);
    } else {
      genActuals(args);
      genBranchAndLink(name);
    }
    currentFrame->setSize(stackPointer);
  }

  /// Member access --------------------------------------------------------//
  std::vector<std::unique_ptr<hexasm::Directive>> &getInstrs() { return instrs; }
  std::vector<std::unique_ptr<hexasm::Directive>> &getData() { return data; }
  void setCurrentFrame(Frame *frame) { currentFrame = frame; }
  void setCurrentFrameSize(size_t size) { currentFrame->setSize(size); }
};

/// Walk the AST and generate intermediate code.
class CodeGen : public AstVisitor {
  SymbolTable &symbolTable;
  CodeBuffer cb;

public:
  CodeGen(SymbolTable &symbolTable) : symbolTable(symbolTable), cb(symbolTable) {}

  void visitPre(Program &tree) {
    // Setup.
    cb.genBR("start"); // Branch to the start.
    cb.genData(SP_INIT_VALUE); // Initialise the stack pointer
    cb.genLabel("start");
    // Branch and link to main().
    cb.genLDAP("_exit");
    cb.genBR("main");
    // Exit.
    cb.genLabel("_exit");
    cb.genLDSPB(); // breg = sp
    cb.genLDAC(0); // areg = 0
    cb.genSTAI(2); // sp[2] = areg
    cb.genLDAC(0);
    cb.genSVC();
  }

  void visitPost(Program &tree) {}

  void visitPre(Proc &proc) {
    // Setup a frame object for the proc/func.
    auto symbol = symbolTable.lookup(proc.getName(), proc.getLocation());
    symbol->setFrame(std::make_unique<Frame>());
    cb.setCurrentFrame(symbol->getFramePtr());
    // Minimum frame size: link + return for a function, link for a process.
    cb.setCurrentFrameSize(proc.isFunction() ? 2 : 1);
    cb.genLabel(proc.getName());
    cb.genPrologue(symbol);
  }

  void visitPost(Proc &proc) {
    auto symbol = symbolTable.lookup(proc.getName(), proc.getLocation());
    cb.genEpilogue(symbol);
  }

  void visitPre(SkipStatement&) {}
  void visitPost(SkipStatement&) {}
  void visitPre(StopStatement&) {}
  void visitPost(StopStatement&) {}
  void visitPre(ReturnStatement&) {}
  void visitPost(ReturnStatement&) {}
  void visitPre(IfStatement&) {}
  void visitPost(IfStatement&) {}
  void visitPre(WhileStatement&) {}
  void visitPost(WhileStatement&) {}
  void visitPre(SeqStatement&) {}
  void visitPost(SeqStatement&) {}
  void visitPre(CallStatement&) {}
  void visitPost(CallStatement &stmt) {
    cb.genCall(stmt.getName(), stmt.getArgs(), false);
  }
  void visitPre(AssStatement&) {}
  void visitPost(AssStatement&) {}

  CodeBuffer &getCodeBuffer() { return cb; }
};

//===---------------------------------------------------------------------===//
// Lower directives.
//===---------------------------------------------------------------------===//

/// Lower the intermediate output produced by code generation into assembly
/// directives that can be consumed by hexasm.
class LowerDirectives {
  std::vector<std::unique_ptr<hexasm::Directive>> instrs;
public:
  LowerDirectives(CodeGen &codeGen) {
    // Merge the instruction and data vectors.
    instrs.insert(instrs.end(),
                  std::make_move_iterator(codeGen.getCodeBuffer().getInstrs().begin()),
                  std::make_move_iterator(codeGen.getCodeBuffer().getInstrs().end()));
    instrs.insert(instrs.end(),
                  std::make_move_iterator(codeGen.getCodeBuffer().getData().begin()),
                  std::make_move_iterator(codeGen.getCodeBuffer().getData().end()));
    // Lower any intermediate directives.
    for (auto it = instrs.begin(); it != instrs.end(); it++) {
      // Lower directive.
      auto token = (*it)->getToken();
      switch (token) {
      case hexasm::Token::PROLOGUE:
        break;
      case hexasm::Token::EPILOGUE:
        break;
      case hexasm::Token::LDAI_FB:
      case hexasm::Token::LDBI_FB:
      case hexasm::Token::STAI_FB: {
        auto oldInstr = dynamic_cast<InstrStackOffset*>((*it).get());
        int newOffset = oldInstr->getFrame()->getSize() - oldInstr->getOffset();
        auto opcode = token == hexasm::Token::LDAI_FB ? hexasm::Token::LDAI :
                      token == hexasm::Token::LDBI_FB ? hexasm::Token::LDBI :
                                                        hexasm::Token::STAI;
        auto newInstr = std::make_unique<hexasm::InstrImm>(opcode, newOffset);
        *it = std::move(newInstr);
        break;
      }
      default: break; // Skip.
      }
    }
  }
  std::vector<std::unique_ptr<hexasm::Directive>> &getInstrs() { return instrs; }
};

} // End namespace xcmp.

#endif // X_CMP_HPP