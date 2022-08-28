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
#include <ostream>
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
  case Token::NOT:         return "~";
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
}

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

struct NonConstArrayLengthError : public Error {
  NonConstArrayLengthError(Location location, std::string name) :
    Error(location, (boost::format("array %s length is not constant") % name).str()) {}
};

struct InvalidSyscallError : public Error {
  InvalidSyscallError(Location location, int sysCallId) :
    Error(location, (boost::format("invalid syscall: %d") % sysCallId).str()) {}
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
class Expr;
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
class AstVisitor {
  bool recurseOp; // Expr
  bool recurseCalls; // Expr
  bool recurseStmts; // Stmts
  // Track the current scope.
  std::stack<std::string> scope;
  // Useful reference "Visitor Pattern, replacing objects" on this strategy.
  // https://softwareengineering.stackexchange.com/questions/313783/visitor-pattern-replacing-objects
  std::unique_ptr<Expr> exprReplacement;

public:
  AstVisitor(bool recurseOp=true, bool recurseCalls=true, bool recurseStmts=true) :
    recurseOp(recurseOp), recurseCalls(recurseCalls), recurseStmts(recurseStmts),
    exprReplacement(nullptr) {}
  bool shouldRecurseOp() const { return recurseOp; }
  bool shouldRecurseCalls() const { return recurseCalls; }
  bool shouldRecurseStmts() const { return recurseStmts; }
  bool hasExprReplacement() const { return exprReplacement != nullptr; }
  void setExprReplacement(std::unique_ptr<Expr> expr) { exprReplacement = std::move(expr); }
  std::unique_ptr<Expr> &getExprReplacement() { return exprReplacement; }
  // Scoping
  void enterProgram() { scope.push(""); }
  void exitProgram() { scope.pop(); }
  void enterProc(const std::string &name) { scope.push(name); }
  void exitProc() { scope.pop(); }
  const std::string getCurrentScope() const {
    assert(scope.size() > 0 && "scope stack empty");
    return scope.top();
  }
  // Vist methods.
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
  AstNode() : location(Location(0, 0)) {}
  AstNode(Location location) : location(location) {}
  virtual ~AstNode() = default;
  virtual void accept(AstVisitor* visitor) = 0;
  const Location &getLocation() const { return location; }
  void replaceExpr(std::unique_ptr<Expr> &expr, AstVisitor *visitor) {
    // If the visitor wishes to replcae the expression, it will have created a
    // replacement, which can be moved in place of the original.
    if (visitor->hasExprReplacement()) {
      expr = std::move(visitor->getExprReplacement());
    }
  }
};

// Expressions ============================================================= //

class Expr : public AstNode {
  std::optional<int> constValue;
public:
  Expr(Location location) : AstNode(location), constValue(std::nullopt) {}
  bool isConst() const { return constValue.has_value(); }
  bool isConstZero() const { return constValue.has_value() && constValue.value() == 0; }
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
    //expr->accept(visitor);
    //replaceExpr(expr, visitor);
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
    replaceExpr(expr, visitor);
    visitor->visitPost(*this);
  }
  const std::string &getName() const { return name; }
  const std::unique_ptr<Expr> &getExpr() { return expr; }
};

class CallExpr : public Expr {
  int sysCallId;
  std::string name;
  std::vector<std::unique_ptr<Expr>> args;
public:
  CallExpr(Location location, int sysCallId) :
      Expr(location), sysCallId(sysCallId) {}
  CallExpr(Location location, int sysCallId, std::vector<std::unique_ptr<Expr>> args) :
      Expr(location), sysCallId(sysCallId), args(std::move(args)) {}
  CallExpr(Location location, std::string name) :
      Expr(location), sysCallId(-1), name(name) {}
  CallExpr(Location location, std::string name, std::vector<std::unique_ptr<Expr>> args) :
      Expr(location), sysCallId(-1), name(name), args(std::move(args)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    if (visitor->shouldRecurseCalls()) {
      for (auto &arg : args) {
        arg->accept(visitor);
        replaceExpr(arg, visitor);
      }
    }
    visitor->visitPost(*this);
  }
  bool isSysCall() const { return sysCallId != -1; }
  int getSysCallId() const { return sysCallId; }
  void setSysCallId(int value) { sysCallId = value; }
  const std::string &getName() const { return name; }
  const std::vector<std::unique_ptr<Expr>> &getArgs() { return args; }
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
    if (!isConst() && visitor->shouldRecurseOp()) {
      element->accept(visitor);
      replaceExpr(element, visitor);
    }
    visitor->visitPost(*this);
  }
  Token getOp() const { return op; }
  std::unique_ptr<Expr> &getElement() { return element; }
};

class BinaryOpExpr : public Expr {
  Token op;
  std::unique_ptr<Expr> LHS, RHS;
public:
  BinaryOpExpr(Location location, Token op, std::unique_ptr<Expr> LHS, std::unique_ptr<Expr> RHS) :
      Expr(location), op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    if (!isConst() && visitor->shouldRecurseOp()) {
      LHS->accept(visitor);
      replaceExpr(LHS, visitor);
      RHS->accept(visitor);
      replaceExpr(RHS, visitor);
    }
    visitor->visitPost(*this);
  }
  Token getOp() const { return op; }
  std::unique_ptr<Expr> &getLHS() { return LHS; }
  std::unique_ptr<Expr> &getRHS() { return RHS; }
  void setLHS(std::unique_ptr<Expr> &newLHS) { LHS = std::move(newLHS); }
  void setRHS(std::unique_ptr<Expr> &newRHS) { RHS = std::move(newRHS); }
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
    replaceExpr(expr, visitor);
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
    replaceExpr(expr, visitor);
    visitor->visitPost(*this);
  }
  int getSize() {
    if (!expr->isConst()) {
      throw NonConstArrayLengthError(getLocation(), getName());
    }
    return expr->getValue();
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
    if (visitor->shouldRecurseStmts()) {
      expr->accept(visitor);
      replaceExpr(expr, visitor);
    }
    visitor->visitPost(*this);
  }
  Expr *getExpr() { return expr.get(); }
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
    if (visitor->shouldRecurseStmts()) {
      condition->accept(visitor);
      replaceExpr(condition, visitor);
      thenStmt->accept(visitor);
      elseStmt->accept(visitor);
    }
    visitor->visitPost(*this);
  }
  const std::unique_ptr<Expr> &getCondition() { return condition; }
  const std::unique_ptr<Statement> &getThenStmt() { return thenStmt; }
  const std::unique_ptr<Statement> &getElseStmt() { return elseStmt; }
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
    if (visitor->shouldRecurseStmts()) {
      condition->accept(visitor);
      replaceExpr(condition, visitor);
      stmt->accept(visitor);
    }
    visitor->visitPost(*this);
  }
  const std::unique_ptr<Expr> &getCondition() { return condition; }
  const std::unique_ptr<Statement> &getStmt() { return stmt; }
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
    if (visitor->shouldRecurseStmts()) {
      // Note that this does not allow replacement of the expr.
      call->accept(visitor);
    }
    visitor->visitPost(*this);
  }
  CallExpr *getCall() { return call.get(); }
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
    if (visitor->shouldRecurseStmts()) {
      LHS->accept(visitor);
      replaceExpr(LHS, visitor);
      RHS->accept(visitor);
      replaceExpr(RHS, visitor);
    }
    visitor->visitPost(*this);
  }
  const std::unique_ptr<Expr> &getLHS() { return LHS; }
  const std::unique_ptr<Expr> &getRHS() { return RHS; }
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
    visitor->enterProc(name);
    for (auto &formal : formals) {
      formal->accept(visitor);
    }
    for (auto &decl : decls) {
      decl->accept(visitor);
    }
    statement->accept(visitor);
    visitor->exitProc();
    visitor->visitPost(*this);
  }
  bool isFunction() const { return function; }
  const std::string &getName() const { return name; }
  std::vector<std::unique_ptr<Formal>> &getFormals() { return formals; }
  std::vector<std::unique_ptr<Decl>> &getDecls() { return decls; }
  const std::unique_ptr<Statement> &getStatement() { return statement; }
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
    visitor->enterProgram();
    for (auto &decl : globalDecls) {
      decl->accept(visitor);
    }
    for (auto &proc : procDecls) {
      proc->accept(visitor);
    }
    visitor->exitProgram();
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
    if (expr.isSysCall()) {
      indent(); outs << boost::format("syscall %d%s\n") % expr.getSysCallId() % locString(expr);
    } else {
      indent(); outs << boost::format("call %s%s\n") % expr.getName() % locString(expr);
    }
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
    if (stmt.getCall()->isSysCall()) {
      indent(); outs << boost::format("syscallstmt %d%s\n") % stmt.getCall()->getSysCallId() % locString(stmt);
    } else {
      indent(); outs << boost::format("callstmt %s\n") % locString(stmt);
    }
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
  ///   <number> "(" <expr-list> ")"
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
    case Token::NUMBER: {
      auto value = lexer.getNumber();
      lexer.getNextToken();
      // System call.
      if (lexer.getLastToken() == Token::LPAREN) {
        if (lexer.getNextToken() == Token::RPAREN) {
          lexer.getNextToken();
          return std::make_unique<CallExpr>(location, value);
        } else {
          auto exprList = parseExprList();
          expect(Token::RPAREN);
          return std::make_unique<CallExpr>(location, value, std::move(exprList));
        }
      } else {
        // Number.
        return std::make_unique<NumberExpr>(location, value);
      }
    }
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
  ///   <identifier> "(" <expr-list> ")"
  ///   <number> "(" [ <expr-list> ")"
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
    case Token::NUMBER: {
      auto element = parseElement();
      // System call
      if (dynamic_cast<CallExpr*>(element.get())) {
        auto callExpr = std::unique_ptr<CallExpr>(static_cast<CallExpr*>(element.release()));
        return std::make_unique<CallStatement>(location, std::move(callExpr));
      } else {
        throw ParserTokenError(location, "invalid statement beginning with number", lexer.getLastToken());
      }
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

class Symbol;

class Frame {
  // The current stack pointer offset from the frame base.  This is a positive
  // index addressing high-to-low memory locations.  Frame base is the top-most
  // memory location of the frame, ie the first location after the caller's
  // frame.
  size_t offset;
  // The running maximum size of the frame.
  size_t size;
  // Exit label.
  std::string exitLabel;

public:
  Frame(std::string exitLabel) : offset(0), size(0), exitLabel(exitLabel) {}
  int getSize() { return size; }
  void incOffset(int amount) {
    offset += amount;
    size = std::max(size, offset); // +1 since it's an offset?
  }
  void decOffset(int amount) {
    offset -= amount;
  }
  void setOffset(int value) { offset = value; }
  size_t getOffset() { return offset; }
  const std::string &getExitLabel() const { return exitLabel; }
};

/// A class to represent a symbol in the program, recording type, scope, AST
/// declaration and Frame infromation where applicable.
class Symbol {
  SymbolType type;
  AstNode *node;
  const std::string scope;
  const std::string name;
  std::shared_ptr<Frame> frame;
  int stackOffset;
  std::string globalLabel;

public:
  Symbol(SymbolType type, AstNode *node, const std::string &scope, const std::string &name) :
      type(type), node(node), scope(scope), name(name) {}
  SymbolType getType() const { return type; }
  const std::string &getScope() const { return scope; }
  AstNode *getNode() const { return node; }
  const std::string &getName() const { return name; }
  void setFrame(std::shared_ptr<Frame> newFrame) { frame = newFrame; }
  Frame *getFrame() { return frame.get(); }
  int getStackOffset() const { return stackOffset; }
  void setStackOffset(int value) { stackOffset = value; }
  const std::string &getGlobalLabel() const { return globalLabel; }
  void setGlobalLabel(const std::string &value) { globalLabel = value; }
};

using SymbolID = std::pair<const std::string, const std::string>;
using SymbolIDRef = std::pair<const std::string&, const std::string&>;

class SymbolTable {
  std::map<SymbolID, std::unique_ptr<Symbol>> symbolMap;

public:
  void insert(SymbolIDRef identifier, std::unique_ptr<Symbol> symbol) {
    //std::cout << "insert " << identifier.first << ", " << identifier.second <<"\n";
    symbolMap[identifier] = std::move(symbol);
  }

  /// Lookup a symbol, and throw an exception if not found.
  Symbol* lookup(SymbolIDRef identifier, const Location &location) {
    //std::cout << "lookup " << identifier.first << ", " << identifier.second <<"\n";
    auto it = symbolMap.find(identifier);
    if (it != symbolMap.end()) {
      return it->second.get();
    }
    // Check the global scope if no match found.
    if (!identifier.first.empty()) {
      it = symbolMap.find(std::make_pair("", identifier.second));
      if (it != symbolMap.end()) {
        return it->second.get();
      }
    }
    throw UnknownSymbolError(location, identifier.second);
  }
};

//===---------------------------------------------------------------------===//
// Symbol table construction.
//===---------------------------------------------------------------------===//

class CreateSymbols : public AstVisitor {
  SymbolTable &symbolTable;
public:
  CreateSymbols(SymbolTable &symbolTable) :
    AstVisitor(false, false, false), symbolTable(symbolTable) {}
  void visitPre(Proc &proc) {
    auto symbolType = proc.isFunction() ? SymbolType::FUNC : SymbolType::PROC;
    symbolTable.insert(std::make_pair(getCurrentScope(), proc.getName()),
                       std::make_unique<Symbol>(symbolType, &proc, getCurrentScope(), proc.getName()));
  }
  void visitPre(ArrayDecl &decl) {
    symbolTable.insert(std::make_pair(getCurrentScope(), decl.getName()),
                       std::make_unique<Symbol>(SymbolType::ARRAY, &decl, getCurrentScope(), decl.getName()));
  }
  void visitPre(VarDecl &decl) {
    symbolTable.insert(std::make_pair(getCurrentScope(), decl.getName()),
                       std::make_unique<Symbol>(SymbolType::VAR, &decl, getCurrentScope(), decl.getName()));
  }
  void visitPre(ValDecl &decl) {
    symbolTable.insert(std::make_pair(getCurrentScope(), decl.getName()),
                       std::make_unique<Symbol>(SymbolType::VAL, &decl, getCurrentScope(), decl.getName()));
  }
  void visitPre(ValFormal &formal) {
    symbolTable.insert(std::make_pair(getCurrentScope(), formal.getName()),
                       std::make_unique<Symbol>(SymbolType::VAL, &formal, getCurrentScope(), formal.getName()));
  }
  void visitPre(ArrayFormal &formal) {
    symbolTable.insert(std::make_pair(getCurrentScope(), formal.getName()),
                       std::make_unique<Symbol>(SymbolType::ARRAY, &formal, getCurrentScope(), formal.getName()));
  }
  void visitPre(ProcFormal &formal) {
    symbolTable.insert(std::make_pair(getCurrentScope(), formal.getName()),
                       std::make_unique<Symbol>(SymbolType::PROC, &formal, getCurrentScope(), formal.getName()));
  }
  void visitPre(FuncFormal &formal) {
    symbolTable.insert(std::make_pair(getCurrentScope(), formal.getName()),
                       std::make_unique<Symbol>(SymbolType::FUNC, &formal, getCurrentScope(), formal.getName()));
  }
};

//===---------------------------------------------------------------------===//
// Constant propagation.
//===---------------------------------------------------------------------===//

class ConstProp : public AstVisitor {
  SymbolTable &symbolTable;
public:
  ConstProp(SymbolTable &symbolTable) :
    AstVisitor(true, true, true), symbolTable(symbolTable) {}
  void visitPost(ValDecl &decl) {
    if (decl.getExpr()->isConst()) {
      decl.setValue(decl.getExpr()->getValue());
    }
  }
  void visitPost(BinaryOpExpr &expr) {
    auto &LHS = expr.getLHS();
    auto &RHS = expr.getRHS();
    if (LHS->isConst() && RHS->isConst()) {
      // Evaluate binary expression.
      int result;
      switch (expr.getOp()) {
        case Token::PLUS:  result = LHS->getValue() +  RHS->getValue(); break;
        case Token::MINUS: result = LHS->getValue() -  RHS->getValue(); break;
        case Token::EQ:    result = LHS->getValue() == RHS->getValue(); break;
        case Token::NE:    result = LHS->getValue() != RHS->getValue(); break;
        case Token::LS:    result = LHS->getValue() <  RHS->getValue(); break;
        case Token::LE:    result = LHS->getValue() <= RHS->getValue(); break;
        case Token::GR:    result = LHS->getValue() >  RHS->getValue(); break;
        case Token::GE:    result = LHS->getValue() >= RHS->getValue(); break;
        case Token::AND:   result = LHS->getValue() == 0 ? 0 : (RHS->getValue() == 0 ? 0 : 1); break;
        case Token::OR:    result = LHS->getValue() != 0 ? 1 : (RHS->getValue() == 0 ? 0 : 1); break;
        default:
          throw SemanticTokenError(expr.getLocation(), "unexpected binary op", expr.getOp());
      }
      expr.setValue(result);
    }
  }
  void visitPost(UnaryOpExpr &expr) {
    auto &element = expr.getElement();
    if (element->isConst()) {
      // Evaluate unary expression.
      int result;
      switch (expr.getOp()) {
        case Token::MINUS: result = -element->getValue(); break;
        case Token::NOT:   result = element->getValue() == 0 ? 1 : 0; break;
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
  void visitPost(CallExpr &expr) {
    // Propagate constant values for syscalls.
    if (!expr.isSysCall()) {
      auto symbol = symbolTable.lookup(std::make_pair(getCurrentScope(), expr.getName()),
                                       expr.getLocation());
      if (auto symbolExpr = dynamic_cast<const ValDecl*>(symbol->getNode())) {
        expr.setSysCallId(symbolExpr->getValue());
      } else {
        return;
      }
    }
    // Check Syscall ID is valid.
    if (expr.getSysCallId() >= static_cast<int>(hex::Syscall::NUM_VALUES) ||
        expr.getSysCallId() < 0) {
      throw InvalidSyscallError(expr.getLocation(), expr.getSysCallId());
    }
  }
  void visitPost(ArraySubscriptExpr &expr) {}
  void visitPost(VarRefExpr &expr) {
    // Propagate constant values to variable references.
    auto symbol = symbolTable.lookup(std::make_pair(getCurrentScope(), expr.getName()),
                                     expr.getLocation());
    if (auto symbolExpr = dynamic_cast<const ValDecl*>(symbol->getNode())) {
      expr.setValue(symbolExpr->getValue());
    }
  }
};

//===---------------------------------------------------------------------===//
// Optimise expressions.
//===---------------------------------------------------------------------===//

class OptimiseExpr : public AstVisitor {
public:
  OptimiseExpr() {}
  void visitPost(BinaryOpExpr &expr) {
    // Translate relational operators ~=, >=, >, <= to expressions only using
    // <, =, ~.
    switch (expr.getOp()) {
      case Token::NE: {
        // LHS ~= RHS -> not(LHS = RHS)
        auto eq = std::make_unique<BinaryOpExpr>(expr.getLocation(), Token::EQ,
                                                 std::move(expr.getLHS()),
                                                 std::move(expr.getRHS()));
        auto replace = std::make_unique<UnaryOpExpr>(expr.getLocation(),
                                                     Token::NOT, std::move(eq));
        setExprReplacement(std::move(replace));
        break;
      }
      case Token::GE: {
        // LHS >= RHS -> not(LHS < RHS)
        auto eq = std::make_unique<BinaryOpExpr>(expr.getLocation(), Token::LS,
                                                 std::move(expr.getLHS()),
                                                 std::move(expr.getRHS()));
        auto replace = std::make_unique<UnaryOpExpr>(expr.getLocation(),
                                                     Token::NOT, std::move(eq));
        setExprReplacement(std::move(replace));
        break;
      }
      case Token::GR: {
        // LHS > RHS -> RHS < LHS
        auto replace = std::make_unique<BinaryOpExpr>(expr.getLocation(), Token::LS,
                                                      std::move(expr.getRHS()),
                                                      std::move(expr.getLHS()));
        setExprReplacement(std::move(replace));
        break;
      }
      case Token::LE: {
        // LHS <= RHS -> not(RHS < LHS)
        auto ls = std::make_unique<BinaryOpExpr>(expr.getLocation(), Token::LS,
                                                 std::move(expr.getRHS()),
                                                 std::move(expr.getLHS()));
        auto replace = std::make_unique<UnaryOpExpr>(expr.getLocation(),
                                                     Token::NOT, std::move(ls));
        setExprReplacement(std::move(replace));
        break;
      }
      default: break;
    }
  }
  void visitPost(UnaryOpExpr &expr) {
    if (!expr.isConst() && expr.getOp() == Token::MINUS) {
      // Transform -x to 0 - x
      auto zero = std::make_unique<NumberExpr>(expr.getLocation(), 0);
      auto replace = std::make_unique<BinaryOpExpr>(expr.getLocation(), expr.getOp(),
                                                    std::move(zero), std::move(expr.getElement()));
      setExprReplacement(std::move(replace));
    }
  }
};

//===---------------------------------------------------------------------===//
// Code generation.
//===---------------------------------------------------------------------===//

const int SP_OFFSET = 1;
const int MAX_ADDRESS = 1 << 16;
const int SP_LINK_VALUE_OFFSET = 0;
const int SP_RETURN_VALUE_OFFSET = 1;
const int FB_PARAM_OFFSET_FUNC = 2;
const int FB_PARAM_OFFSET_PROC = 1;

enum class Reg { A, B };

class IntermediateDirective : public hexasm::Directive {
public:
  IntermediateDirective(hexasm::Token token) : hexasm::Directive(token) {}
  bool operandIsLabel() const { return false; }
  size_t getSize() const { return 0; }
  int getValue() const { return 0; }
};

class SPValue : public IntermediateDirective {
public:
  SPValue() : IntermediateDirective(hexasm::Token::SP_VALUE) {}
  std::string toString() const { return "SP_VALUE"; }
};

/// Procedure/function prologue.
class Prologue : public IntermediateDirective {
  Symbol *symbol;
public:
  Prologue(Symbol *symbol) : IntermediateDirective(hexasm::Token::PROLOGUE), symbol(symbol) {}
  std::string toString() const { return "PROLOGUE " + symbol->getName(); }
  Symbol *getSymbol() { return symbol; }
  Frame *getFrame() { return symbol->getFrame(); }
};

/// Procedure/function epilogue.
class Epilogue : public IntermediateDirective {
  Symbol *symbol;
public:
  Epilogue(Symbol *symbol) : IntermediateDirective(hexasm::Token::EPILOGUE), symbol(symbol) {}
  std::string toString() const { return "EPILOGUE " + symbol->getName(); }
  Symbol *getSymbol() { return symbol; }
  Frame *getFrame() { return symbol->getFrame(); }
};

/// Instruction relative offset to stack frame base.
class InstrStackOffset : public hexasm::InstrImm {
  Frame *frame;
  int offset;
public:
  InstrStackOffset(hexasm::Token token, Frame *frame, int offset) :
      InstrImm(token, 0), frame(frame), offset(offset) {}
  std::string toString() const {
    return (boost::format("%s %d") % hexasm::tokenEnumStr(getToken()) % offset).str();
  }
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
  CodeBuffer(SymbolTable &symbolTable) :
    symbolTable(symbolTable), constCount(0), stringCount(0), labelCount(0) {}

  const std::string getLabel() { return std::string("lab") + std::to_string(labelCount++); }
  void insertInstr(std::unique_ptr<hexasm::Directive> instr) { instrs.push_back(std::move(instr)); }
  void insertData(std::unique_ptr<hexasm::Directive> data) { instrs.push_back(std::move(data)); }

  /// Directive generation -------------------------------------------------///
  void genData(uint32_t value)        { data.push_back(std::make_unique<hexasm::Data>(hexasm::Token::DATA, value)); }
  void genDataLabel(std::string name) { data.push_back(std::make_unique<hexasm::Label>(hexasm::Token::IDENTIFIER, name)); }
  void genInstrData(uint32_t value)   { instrs.push_back(std::make_unique<hexasm::Data>(hexasm::Token::DATA, value)); }
  void genLabel(std::string name)     { instrs.push_back(std::make_unique<hexasm::Label>(hexasm::Token::IDENTIFIER, name)); }
  void genFunc(std::string name)      { instrs.push_back(std::make_unique<hexasm::Func>(hexasm::Token::FUNC, name)); }
  void genProc(std::string name)      { instrs.push_back(std::make_unique<hexasm::Proc>(hexasm::Token::PROC, name)); }

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
  void genLDAC(std::string label) { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::LDAC, label, false)); }
  void genLDBC(std::string label) { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::LDBC, label, true)); }
  void genLDAP(std::string label) { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::LDAP, label, true)); }
  void genLDAI(int value)         { instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::LDAI, value)); }
  void genLDBI(int value)         { instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::LDBI, value)); }
  void genSTAI(int value)         { instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::STAI, value)); }
  void genBR(std::string label)   { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::BR, label, true)); }
  void genBRZ(std::string label)  { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::BRZ, label, true)); }
  void genBRN(std::string label)  { instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::BRN, label, true)); }
  void genOPR(hexasm::Token op)   { instrs.push_back(std::make_unique<hexasm::InstrOp>(hexasm::Token::OPR, op)); }

  /// Intermediate instruction for placeholder SP value --------------------///
  void genSPValue() { instrs.push_back(std::make_unique<SPValue>()); }

  /// Intermediate instructions for procedure calling ----------------------///
  void genPrologue(Symbol *symbol) { instrs.push_back(std::make_unique<Prologue>(symbol)); }
  void genEpilogue(Symbol *symbol) { instrs.push_back(std::make_unique<Epilogue>(symbol)); }

  /// Intermediate instructions for frame-base relative accesses -----------///
  void genLDAI_FB(Frame *frame, int offset) { instrs.push_back(std::make_unique<InstrStackOffset>(hexasm::Token::LDAI_FB, frame, offset)); }
  void genLDBI_FB(Frame *frame, int offset) { instrs.push_back(std::make_unique<InstrStackOffset>(hexasm::Token::LDBI_FB, frame, offset)); }
  void genSTAI_FB(Frame *frame, int offset) { instrs.push_back(std::make_unique<InstrStackOffset>(hexasm::Token::STAI_FB, frame, offset)); }

  /// Helpers --------------------------------------------------------------///
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
    const std::string &currentScope;
    Reg reg;
  public:
    ExprCodeGen(SymbolTable &st, CodeBuffer &cb,
                const std::string &currentScope, Reg reg) :
      AstVisitor(false, false, false), st(st), cb(cb),
      currentScope(currentScope), reg(reg) {}
    /// Return true if the expr needs to be materialised in an A register.
    bool needsAReg(std::unique_ptr<Expr> &expr) {
      return !(expr->isConst() || dynamic_cast<StringExpr*>(expr.get()) || dynamic_cast<VarRefExpr*>(expr.get()));
    }
    void genBinopOperands(BinaryOpExpr &expr) {
      // For ADD and SUB binary operations:
      //  - LHS materialise in areg.
      //  - RHS materialise in breg.
      // NOTE: this does not respect left-to-right ordering of operations.
      if (needsAReg(expr.getRHS())) {
        // Switch RHS areg into breg after generating LHS.
        auto currentFrame = cb.getCurrentFrame();
        size_t stackOffset = cb.getCurrentFrame()->getOffset();
        // Gen RHS and save to stack.
        cb.genExpr(expr.getRHS(), currentScope);
        auto offset = currentFrame->getOffset();
        currentFrame->incOffset(1);
        cb.genLDBM(SP_OFFSET);
        cb.genSTAI_FB(currentFrame, -offset);
        // Gen LHS.
        cb.genExpr(expr.getLHS(), currentScope);
        // Restore RHS from stack into breg.
        cb.genLDBM(SP_OFFSET);
        cb.genLDBI_FB(currentFrame, -offset);
        currentFrame->setOffset(stackOffset);
      } else {
        cb.genExpr(expr.getLHS(), currentScope);
        cb.genExpr(expr.getRHS(), currentScope, Reg::B);
      }
    }
    void visitPost(BinaryOpExpr &expr) {
      if (expr.isConst()) {
        cb.genConst(Reg::A, expr.getValue());
      } else {
        // Generate a binary op.
        switch (expr.getOp()) {
          case Token::PLUS:
            genBinopOperands(expr);
            cb.genADD();
            break;
          case Token::MINUS:
            genBinopOperands(expr);
            cb.genSUB();
            break;
          case Token::AND: {
            // Logical AND of operands. If first operand is false, result is
            // false, otherwise the result is the value of the second operand.
            auto endLabel = cb.getLabel();
            cb.genExpr(expr.getLHS(), currentScope);
            cb.genBRZ(endLabel);
            cb.genExpr(expr.getRHS(), currentScope);
            cb.genLabel(endLabel);
            break;
          }
          case Token::OR: {
            // Logical OR of operands. If the value of the first operand is
            // true, the result is true, otherwise the result is the value of
            // the second operand.
            auto falseLabel = cb.getLabel();
            auto endLabel = cb.getLabel();
            cb.genExpr(expr.getLHS(), currentScope);
            cb.genBRZ(falseLabel);
            cb.genBR(endLabel);
            cb.genLabel(falseLabel);
            cb.genExpr(expr.getRHS(), currentScope);
            cb.genLabel(endLabel);
            break;
          }
          case Token::EQ: {
            if (expr.getLHS()->isConstZero()) {
              cb.genExpr(expr.getRHS(), currentScope);
            }else if (expr.getRHS()->isConstZero()) {
              cb.genExpr(expr.getLHS(), currentScope);
            } else {
              // Create a new AST node on the fly to generate the expression
              // 'LHS - RHS'. Note that this modifies the program's AST,
              // precluding this code from running again.
              auto subtract = std::make_unique<BinaryOpExpr>(expr.getLocation(), Token::MINUS,
                                                             std::move(expr.getLHS()),
                                                             std::move(expr.getRHS()));
              cb.genExpr(subtract.get(), currentScope);
            }
            auto trueLabel = cb.getLabel();
            auto endLabel = cb.getLabel();
            cb.genBRZ(trueLabel); // Equal if result is zero.
            cb.genLDAC(0);
            cb.genBR(endLabel);
            cb.genLabel(trueLabel);
            cb.genLDAC(1);
            cb.genLabel(endLabel);
            break;
          }
          case Token::LS: {
            // LHS < RHS -> LHS - RHS < 0
            if (expr.getRHS()->isConstZero()) {
              // If RHS is zero, then only consider is LHS is negative.
              cb.genExpr(expr.getLHS(), currentScope);
            } else {
              // Compute LHS - RHS by creating a subtraction AST node.
              auto subtract = std::make_unique<BinaryOpExpr>(expr.getLocation(), Token::MINUS,
                                                             std::move(expr.getLHS()),
                                                             std::move(expr.getRHS()));
              cb.genExpr(subtract.get(), currentScope);
              // Restore the operand pointers.
              expr.setLHS(subtract->getLHS());
              expr.setRHS(subtract->getRHS());
            }
            auto trueLabel = cb.getLabel();
            auto endLabel = cb.getLabel();
            cb.genBRN(trueLabel);
            cb.genLDAC(0);
            cb.genBR(endLabel);
            cb.genLabel(trueLabel);
            cb.genLDAC(1);
            cb.genLabel(endLabel);
            break;
          }
          default:
            assert(0 && "unexpected token in binop codegen");
            break;
        }
      }
    }
    void visitPost(UnaryOpExpr &expr) {
      if (expr.isConst()) {
        cb.genConst(reg, expr.getValue());
      } else {
        // Generate unary logical ~ op.
        if (expr.getOp() == Token::NOT) {
          auto trueLabel = cb.getLabel();
          auto endLabel = cb.getLabel();
          cb.genExpr(expr.getElement(), currentScope);
          cb.genBRZ(trueLabel); // False -> True
          cb.genLDAC(0);
          cb.genBR(endLabel);
          cb.genLabel(trueLabel);
          cb.genLDAC(1);
          cb.genLabel(endLabel);
        } else {
          assert(0 && "unexpected token in unary op codegen");
        }
      }
    }
    void visitPost(StringExpr &expr) {
      cb.genString(reg, expr.getValue());
    }
    void visitPost(BooleanExpr &expr) {
      cb.genConst(reg, expr.getValue());
    }
    void visitPost(NumberExpr &expr) {
      cb.genConst(reg, expr.getValue());
    }
    void visitPost(CallExpr &expr) {
      if (expr.isSysCall()) {
        cb.genSysCall(expr.getSysCallId(), expr.getArgs(), currentScope);
      } else {
        auto symbol = st.lookup(std::make_pair(currentScope, expr.getName()),
                                expr.getLocation());
        if (symbol->getType() == SymbolType::FUNC) {
          cb.genFuncCall(expr.getName(), expr.getArgs(), currentScope);
        } else {
          cb.genProcCall(expr.getName(), expr.getArgs(), currentScope);
        }
      }
      // The recursion must halt here, which is controlled by
      // AstVisitor::shouldRecurseCalls().
    }
    void visitPost(ArraySubscriptExpr &expr) {
      // Generate array subscript.
      auto baseSymbol = st.lookup(std::make_pair(currentScope, expr.getName()),
                                                 expr.getLocation());
      if (expr.getExpr()->isConst()) {
        cb.genVar(Reg::A, baseSymbol);
        cb.genLDAI(expr.getExpr()->getValue());
      } else {
        cb.genVar(Reg::B, baseSymbol);
        cb.genExpr(expr.getExpr(), currentScope);
        cb.genADD();
        cb.genLDAI(0);
      }
    }
    void visitPost(VarRefExpr &expr) {
      if (expr.isConst()) {
        cb.genConst(reg, expr.getValue());
      } else {
        cb.genVar(reg, st.lookup(std::make_pair(currentScope, expr.getName()),
                                 expr.getLocation()));
      }
    }
  };

  class StmtCodeGen : public AstVisitor {
    SymbolTable &st;
    CodeBuffer &cb;
    const std::string &currentScope;
  public:
    StmtCodeGen(SymbolTable &st, CodeBuffer &cb, const std::string &currentScope) :
      AstVisitor(false, false, false), st(st), cb(cb), currentScope(currentScope) {}

    void visitPost(SkipStatement&) {
      // Do nothing.
    }

    void visitPost(StopStatement&) {
      // SVC exit 0
      cb.genLDBM(SP_OFFSET); // breg = sp
      cb.genLDAC(0); // areg = 0
      cb.genSTAI(FB_PARAM_OFFSET_FUNC); // sp[2] = areg (actual param 0)
      cb.genSVC();
    }

    void visitPost(ReturnStatement &stmt) {
      // TODO: Check a process cannot contain a return.
      // TODO: Check a function must end with a return.
      // TODO: handle tail function calls
      cb.genExpr(stmt.getExpr(), currentScope);
      cb.genBR(cb.getCurrentFrame()->getExitLabel());
    }

    void visitPost(IfStatement &stmt) {
      bool skipThen = dynamic_cast<SkipStatement*>(stmt.getThenStmt().get());
      bool skipElse = dynamic_cast<SkipStatement*>(stmt.getElseStmt().get());
      if (skipThen && skipElse) {
        // Do nothing.
      } else if (skipElse) {
        // No else branch.
        auto endLabel = cb.getLabel();
        cb.genExpr(stmt.getCondition(), currentScope);
        cb.genBRZ(endLabel);
        cb.genStmt(stmt.getThenStmt(), currentScope);
        cb.genLabel(endLabel);
      } else if (skipThen) {
        // No then branch.
        auto elseLabel = cb.getLabel();
        auto endLabel = cb.getLabel();
        cb.genExpr(stmt.getCondition(), currentScope);
        cb.genBRZ(elseLabel);
        cb.genBR(endLabel);
        cb.genLabel(elseLabel);
        cb.genStmt(stmt.getElseStmt(), currentScope);
        cb.genLabel(endLabel);
      } else {
        auto elseLabel = cb.getLabel();
        auto endLabel = cb.getLabel();
        cb.genExpr(stmt.getCondition(), currentScope);
        cb.genBRZ(elseLabel);
        cb.genStmt(stmt.getThenStmt(), currentScope);
        cb.genBR(endLabel);
        cb.genLabel(elseLabel);
        cb.genStmt(stmt.getElseStmt(), currentScope);
        cb.genLabel(endLabel);
      }
    }

    void visitPost(WhileStatement &expr) {
      auto beginLabel = cb.getLabel();
      auto endLabel = cb.getLabel();
      cb.genLabel(beginLabel);
      cb.genExpr(expr.getCondition(), currentScope);
      cb.genBRZ(endLabel);
      cb.genStmt(expr.getStmt(), currentScope);
      cb.genBR(beginLabel);
      cb.genLabel(endLabel);
    }

    void visitPost(SeqStatement&) {
      // Handled by the visitor.
    }

    void visitPost(CallStatement &stmt) {
      if (stmt.getCall()->isSysCall()) {
        cb.genSysCall(stmt.getCall()->getSysCallId(), stmt.getCall()->getArgs(), currentScope);
      } else {
        cb.genProcCall(stmt.getCall()->getName(), stmt.getCall()->getArgs(), currentScope);
      }
    }

    void visitPost(AssStatement &expr) {
      if (auto *varRefLHS = dynamic_cast<VarRefExpr*>(expr.getLHS().get())) {
        // Generate RHS value into areg.
        cb.genExpr(expr.getRHS(), currentScope);
        // LHS variable reference.
        auto symbol = st.lookup(std::make_pair(currentScope, varRefLHS->getName()),
                                expr.getLocation());
        if (symbol->getScope().empty()) {
          // Global scope.
          cb.genSTAM(symbol->getGlobalLabel());
        } else {
          // Local scope.
          cb.genLDBM(SP_OFFSET);
          cb.genSTAI_FB(cb.getCurrentFrame(), symbol->getStackOffset());
        }
      } else if (auto *arraySubLHS = dynamic_cast<ArraySubscriptExpr*>(expr.getLHS().get())) {
        // Handle LHS subscript.
        // Note that arrays are always global.
        // Generate the array element address and save it to the stack.
        cb.genVar(Reg::B, st.lookup(std::make_pair(currentScope, arraySubLHS->getName()),
                                    arraySubLHS->getLocation()));
        cb.genExpr(arraySubLHS->getExpr(), currentScope);
        cb.genADD();
        auto stackOffset = cb.getCurrentFrame()->getOffset();
        cb.getCurrentFrame()->incOffset(1);
        cb.genLDBM(SP_OFFSET);
        cb.genSTAI_FB(cb.getCurrentFrame(), -stackOffset);
        // Generate the RHS expression.
        cb.genExpr(expr.getRHS(), currentScope);
        // Load the array address into breg.
        cb.genLDBM(SP_OFFSET);
        cb.genLDBI_FB(cb.getCurrentFrame(), -stackOffset);
        // Save areg into mem[breg].
        cb.genSTAI(0);
        cb.getCurrentFrame()->decOffset(1);
      } else {
        assert(0 && "unexpected target of assignment statement");
      }
    }
  };

  /// Generate code for an expression using the ExprCodeGen visitor.
  void genExpr(const std::unique_ptr<Expr> &expr,
               const std::string &currentScope,
               Reg reg=Reg::A) {
    ExprCodeGen visitor(symbolTable, *this, currentScope, reg);
    expr->accept(&visitor);
  }

  void genExpr(Expr *expr, const std::string &currentScope,
               Reg reg=Reg::A) {
    ExprCodeGen visitor(symbolTable, *this, currentScope, reg);
    expr->accept(&visitor);
  }

  /// Generate code for a statement using the StmtCodeGen visitor.
  void genStmt(const std::unique_ptr<Statement> &stmt,
               const std::string &currentScope) {
    StmtCodeGen visitor(symbolTable, *this, currentScope);
    stmt->accept(&visitor);
  }

  /// Return true if the expression contains a call.
  bool containsCall(const std::unique_ptr<Expr> &expr) {
    ContainsCall visitor;
    expr->accept(&visitor);
    return visitor.getFlag();
  }

  /// Code generation ------------------------------------------------------///

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
      case Reg::A: genLDAM(label); break;
      case Reg::B: genLDBM(label); break;
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
    packedWord |= value.size() & 0xFF;
    for (size_t strByteIndex = 0; strByteIndex < value.size(); strByteIndex++) {
      auto bytePos = (strByteIndex + 1) % 4;
      packedWord |= value[strByteIndex] << (bytePos * 8);
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
    if (symbol->getScope().empty()) {
      // Load from globals.
      switch (reg) {
      case Reg::A:
        genLDAM(symbol->getGlobalLabel());
        break;
      case Reg::B:
        genLDBM(symbol->getGlobalLabel());
        break;
      }
    } else {
      // Load from stack.
      switch (reg) {
      case Reg::A:
        genLDAM(SP_OFFSET);
        genLDAI_FB(symbol->getFrame(), symbol->getStackOffset());
        break;
      case Reg::B:
        genLDBM(SP_OFFSET);
        genLDBI_FB(symbol->getFrame(), symbol->getStackOffset());
        break;
      }
    }
  }

  /// Generate actual parameters that contain calls.
  void genCallActuals(const std::vector<std::unique_ptr<Expr>> &args,
                      const std::string &currentScope) {
    size_t stackOffset = currentFrame->getOffset();
    for (auto &arg : args) {
      if (containsCall(arg)) {
        // For each actual expression containing one or more calls, allocate a
        // stack word (FB relative) for the result of that call since it cannot
        // be written directly into the parameter slots until all calls have
        // been resolved.
        genExpr(arg.get(), currentScope);
        genLDBM(SP_OFFSET);
        genSTAI_FB(currentFrame, -currentFrame->getOffset());
        currentFrame->incOffset(1);
      }
    }
    // Restore the stack pointer offset so loadActuals can sequence through
    // the call actual locations again.
    currentFrame->setOffset(stackOffset);
  }

  void loadActuals(const std::vector<std::unique_ptr<Expr>> &args, size_t parameterOffset,
                   const std::string &currentScope) {
    size_t parameterIndex = parameterOffset;
    for (auto &arg : args) {
      if (containsCall(arg)) {
        // For each actual expression containing one or more calls, load the
        // expression value saved to a temporary stack location and store it
        // to the actual parameter location.
        genLDAM(SP_OFFSET);
        genLDAI_FB(currentFrame, -currentFrame->getOffset());
        currentFrame->incOffset(1);
        genLDBM(SP_OFFSET);
        genSTAI(parameterIndex);
      } else {
        // For all other actual expressions, generate the value and store it to
        // the actual parameter location.
        genExpr(arg.get(), currentScope);
        genLDBM(SP_OFFSET);
        genSTAI(parameterIndex);
      }
      parameterIndex++;
    }
  }

  void genSysCall(int syscallId, const std::vector<std::unique_ptr<Expr>> &args,
                  const std::string &currentScope) {
    auto stackOffset = currentFrame->getOffset();
    // Actual parameters.
    genCallActuals(args, currentScope);
    loadActuals(args, FB_PARAM_OFFSET_FUNC, currentScope);
    currentFrame->incOffset(args.size() + FB_PARAM_OFFSET_FUNC);
    // Perform syscall.
    genLDAC(syscallId);
    genOPR(hexasm::Token::SVC);
    // Load any return value into areg.
    genLDAM(SP_OFFSET);
    genLDAI(SP_RETURN_VALUE_OFFSET);
    currentFrame->setOffset(stackOffset);
  }

  void genFuncCall(const std::string &name, const std::vector<std::unique_ptr<Expr>> &args,
                   const std::string &currentScope) {
    auto stackOffset = currentFrame->getOffset();
    // Actual parameters.
    genCallActuals(args, currentScope);
    loadActuals(args, FB_PARAM_OFFSET_FUNC, currentScope);
    currentFrame->incOffset(args.size() + FB_PARAM_OFFSET_FUNC);
    // Branch and link.
    auto linkLabel = getLabel();
    genLDAP(linkLabel);
    genBR(name);
    genLabel(linkLabel);
    // Load the result of the function call into areg.
    genLDAM(SP_OFFSET);
    genLDAI(SP_RETURN_VALUE_OFFSET);
    currentFrame->setOffset(stackOffset);
  }

  void genProcCall(const std::string &name, const std::vector<std::unique_ptr<Expr>> &args,
                   const std::string &currentScope) {
    auto stackOffset = currentFrame->getOffset();
    // Actual parameters.
    genCallActuals(args, currentScope);
    loadActuals(args, FB_PARAM_OFFSET_PROC, currentScope);
    currentFrame->incOffset(args.size() + FB_PARAM_OFFSET_PROC);
    // Branch and link.
    auto linkLabel = getLabel();
    genLDAP(linkLabel);
    genBR(name);
    genLabel(linkLabel);
    currentFrame->setOffset(stackOffset);
  }

  /// Reporting -------------------------------------------------------------//
  void emitInstrs(std::ostream &out) {
    for (auto &directive : instrs) {
      if (directive->getToken() == hexasm::Token::PROC ||
          directive->getToken() == hexasm::Token::FUNC ||
          directive->getToken() == hexasm::Token::PROLOGUE) {
        // Add some new lines for these section markers.
        out << boost::format("\n%-20s\n") % directive->toString();
      } else if (directive->getToken() == hexasm::Token::SP_VALUE) {
        // SP value and data section are emitted together.
        out << boost::format("%-20s\n") % directive->toString();
        for (auto &directive : data) {
          out << boost::format("%-20s\n") % directive->toString();
        }
      } else {
        // All other directives.
        out << boost::format("%-20s\n") % directive->toString();
      }
    }
    out << "\n";
  }

  /// Member access --------------------------------------------------------//
  std::vector<std::unique_ptr<hexasm::Directive>> &getInstrs() { return instrs; }
  std::vector<std::unique_ptr<hexasm::Directive>> &getData() { return data; }
  void setCurrentFrame(Frame *frame) { currentFrame = frame; }
  Frame *getCurrentFrame() { return currentFrame; }
};

/// Assign stack locations to function/process formal parameters, represented
/// by frame-base offsets into the previous (caller) frame. Offset by 1 for
/// processes to account for the link address slot, 2 for functions for
/// the link and return value slots, and +1 in both cases to account for the
/// frame-base indexing.
class FormalLocations : public AstVisitor {
  SymbolTable &st;
  const std::string &currentScope;
  std::shared_ptr<Frame> &frame;
  size_t frameBaseOffset;
  void assignLocation(Formal &formal) {
    auto symbol = st.lookup(std::make_pair(currentScope, formal.getName()),
                            formal.getLocation());
    symbol->setStackOffset(frameBaseOffset++);
    symbol->setFrame(frame);
  }
public:
  FormalLocations(SymbolTable &st, const std::string &currentScope,
                  std::shared_ptr<Frame> &frame, bool isFunction) :
    AstVisitor(false, false, false), st(st), currentScope(currentScope), frame(frame),
    frameBaseOffset(1 + (isFunction ? FB_PARAM_OFFSET_FUNC : FB_PARAM_OFFSET_PROC)) {}
  void visitPost(ValFormal &formal) { assignLocation(formal); }
  void visitPost(ArrayFormal &formal) { assignLocation(formal); }
  void visitPost(ProcFormal &formal) { assignLocation(formal); }
  void visitPost(FuncFormal &formal) { assignLocation(formal); }
};

/// Assign stack locations to local variables, starting from the base of the frame.
class LocalDeclLocations : public AstVisitor {
  SymbolTable &st;
  const std::string &currentScope;
  std::shared_ptr<Frame> &frame;
  size_t count;
  void assignLocation(Decl &decl, size_t size) {
    auto symbol = st.lookup(std::make_pair(currentScope, decl.getName()),
                            decl.getLocation());
    symbol->setStackOffset(-count);
    symbol->setFrame(frame);
    frame->incOffset(size);
    count += size;
  }
public:
  LocalDeclLocations(SymbolTable &st, const std::string &currentScope,
                     std::shared_ptr<Frame> &frame) :
    AstVisitor(false, false, false), st(st), currentScope(currentScope),
    frame(frame), count(0) {}
  void visitPost(ArrayDecl &decl) { assignLocation(decl, decl.getSize()); }
  void visitPost(VarDecl &decl) { assignLocation(decl, 1); }
  void visitPost(ValDecl &decl) { assignLocation(decl, 1); }
};

/// Walk the AST and generate intermediate code.
class CodeGen : public AstVisitor {
  SymbolTable &st;
  CodeBuffer cb;
  size_t globalsOffset;

public:
  CodeGen(SymbolTable &symbolTable) :
    AstVisitor(false, false, false), st(symbolTable), cb(symbolTable),
    globalsOffset(0) {}

  void visitPre(Program &tree) {
    // Setup.
    cb.genBR("start"); // Branch to the start.
    cb.genSPValue(); // Placeholder for the stack pointer value.
    cb.genLabel("start");
    // Branch and link to main().
    cb.genLDAP("_exit");
    cb.genBR("main");
    // Exit.
    cb.genLabel("_exit");
    cb.genLDBM(SP_OFFSET); // breg = sp
    cb.genLDAC(0); // areg = 0
    cb.genSTAI(FB_PARAM_OFFSET_FUNC); // sp[2] = areg (actual param 0)
    cb.genSVC();
  }

  void visitPost(Program &tree) {}

  /// Procedure call setup.
  void visitPre(Proc &proc) {
    // Setup a frame object for the proc/func.
    auto symbol = st.lookup(std::make_pair(getCurrentScope(), proc.getName()),
                            proc.getLocation());
    auto frame = std::make_shared<Frame>(cb.getLabel());
    symbol->setFrame(frame);
    cb.setCurrentFrame(symbol->getFrame());
    // Allocate storage locations to formals.
    FormalLocations formalLocations(st, proc.getName(), frame, proc.isFunction());
    proc.accept(&formalLocations);
    // Allocate storage locations to local declarations.
    LocalDeclLocations localDeclLocations(st, proc.getName(), frame);
    proc.accept(&localDeclLocations);
    // Generate the prologue.
    cb.genPrologue(symbol);
    // Generate the body.
    cb.genStmt(proc.getStatement(), proc.getName());
  }

  /// Proceudre call completion.
  void visitPost(Proc &proc) {
    auto symbol = st.lookup(std::make_pair(getCurrentScope(), proc.getName()),
                            proc.getLocation());
    cb.genEpilogue(symbol);
  }

  /// Global variables. Allocate a word with a DATA directive and assign them
  /// a label.
  void visitPost(VarDecl &decl) {
    auto symbol = st.lookup(std::make_pair(getCurrentScope(), decl.getName()),
                            decl.getLocation());
    auto label = cb.getLabel();
    symbol->setGlobalLabel(label);
    cb.genDataLabel(label);
    cb.genData(0);
  }

  /// Global arrays. Allocate space at the end of memory, generate a DATA
  /// directive with the address and assign it a label.
  void visitPost(ArrayDecl &decl) {
    auto symbol = st.lookup(std::make_pair(getCurrentScope(), decl.getName()),
                            decl.getLocation());
    globalsOffset += decl.getSize();
    size_t address = MAX_ADDRESS - globalsOffset;
    auto label = cb.getLabel();
    symbol->setGlobalLabel(label);
    cb.genDataLabel(label);
    cb.genData(address);
  }

  /// Member access --------------------------------------------------------//
  CodeBuffer &getCodeBuffer() { return cb; }
  size_t getGlobalsOffset() const { return globalsOffset; }
  void emitInstrs(std::ostream &out) { cb.emitInstrs(out); }
};

//===---------------------------------------------------------------------===//
// Lower directives.
//===---------------------------------------------------------------------===//

/// Lower the intermediate output produced by code generation into assembly
/// directives that can be consumed by hexasm.
class LowerDirectives {
  CodeBuffer cb;

public:
  LowerDirectives(SymbolTable &symbolTable, CodeGen &cg) : cb(symbolTable) {
    // Lower intermediate instruction directives.
    for (auto &instr : cg.getCodeBuffer().getInstrs()) {
      auto token = instr->getToken();
      switch (token) {
      case hexasm::Token::SP_VALUE: {
        // SP value.
        cb.genInstrData(MAX_ADDRESS - cg.getGlobalsOffset() - 1);
        // Emit data directives for globals, constants and strings.
        for (auto &data : cg.getCodeBuffer().getData()) {
          cb.insertInstr(std::move(data));
        }
        break;
      }
      case hexasm::Token::PROLOGUE: {
        auto prologue = dynamic_cast<Prologue*>(instr.get());
        auto name = prologue->getSymbol()->getName();
        if (prologue->getSymbol()->getType() == SymbolType::FUNC) {
          cb.genFunc(name);
        }
        if (prologue->getSymbol()->getType() == SymbolType::PROC) {
          cb.genProc(name);
        }
        // Save current stack pointer.
        cb.genLDBM(SP_OFFSET);
        cb.genSTAI(0);
        if (prologue->getFrame()->getSize() > 0) {
          // Extend the stack pointer by the frame size.
          cb.genLDAC(-prologue->getFrame()->getSize());
          cb.genOPR(hexasm::Token::ADD);
          cb.genSTAM(SP_OFFSET);
        }
        break;
      }
      case hexasm::Token::EPILOGUE: {
        auto epilogue = dynamic_cast<Epilogue*>(instr.get());
        auto frameSize = epilogue->getFrame()->getSize();
        cb.genLabel(epilogue->getFrame()->getExitLabel());
        // Function
        if (epilogue->getSymbol()->getType() == SymbolType::FUNC) {
          // Store return value in areg.
          cb.genLDBM(SP_OFFSET);
          cb.genSTAI(frameSize + 1);
          if (epilogue->getFrame()->getSize() > 0) {
            // Contract the stack poiner.
            cb.genLDAC(frameSize);
            cb.genOPR(hexasm::Token::ADD);
            cb.genSTAM(SP_OFFSET);
          }
          // Branch back to the caller.
          // breg holds unadjusted SP.
          cb.genLDBI(frameSize);
          cb.genOPR(hexasm::Token::BRB);
          break;
        }
        // Process
        if (epilogue->getSymbol()->getType() == SymbolType::PROC) {
          if (epilogue->getFrame()->getSize() > 0) {
            // Contract the stack poiner.
            cb.genLDBM(SP_OFFSET);
            cb.genLDAC(frameSize);
            cb.genOPR(hexasm::Token::ADD);
            cb.genSTAM(SP_OFFSET);
          }
          // Branch back to the caller.
          // breg holds unadjusted SP.
          cb.genLDBI(frameSize);
          cb.genOPR(hexasm::Token::BRB);
          break;
        }
      }
      case hexasm::Token::LDAI_FB:
      case hexasm::Token::LDBI_FB:
      case hexasm::Token::STAI_FB: {
        auto oldInstr = dynamic_cast<InstrStackOffset*>(instr.get());
        // Calculate the new offset from the SP: frame size plus frame-base
        // offset (negative to access current frame, positive to access
        // previous frame).
        int newOffset = oldInstr->getFrame()->getSize() - 1 + oldInstr->getOffset();
        switch (token) {
        case hexasm::Token::LDAI_FB: cb.genLDAI(newOffset); break;
        case hexasm::Token::LDBI_FB: cb.genLDBI(newOffset); break;
        case hexasm::Token::STAI_FB: cb.genSTAI(newOffset); break;
        default:
          assert(0 && "unexpected token in lowering of FB-relative memory accesses");
          break;
        }
        break;
      }
      default:
        // Otherwise just copy the directive.
        cb.insertInstr(std::move(instr));
        break;
      }
    }
  }

  /// Reporting -------------------------------------------------------------//
  void emitInstrs(std::ostream &out) { cb.emitInstrs(out); }

  /// Member reporting ------------------------------------------------------//
  std::vector<std::unique_ptr<hexasm::Directive>> &getInstrs() {
    return cb.getInstrs();
  }
};

//===---------------------------------------------------------------------===//
// Report frame contents.
//===---------------------------------------------------------------------===//

class ReportMemoryInfo : public AstVisitor {
  SymbolTable &st;
  const std::vector<std::unique_ptr<hexasm::Directive>> &directives;
  std::ostream &outs;
  void reportFrame(Frame *frame, Proc &proc) {
    outs << boost::format("Frame for %s\n") % proc.getName();
    outs << "  Size: " << frame->getSize() << "\n";
    if (proc.getDecls().empty()) {
      outs << "  No local variables\n";
    } else {
      outs << "  Formals:\n";
      for (auto &decl : proc.getFormals()) {
        auto symbol = st.lookup(std::make_pair(proc.getName(), decl->getName()),
                                decl->getLocation());
        auto index = symbol->getFrame()->getSize() - 1 + symbol->getStackOffset();
        outs << boost::format("    %s at index %d\n") % symbol->getName() % index;
      }
      outs << "  Locals:\n";
      for (auto &decl : proc.getDecls()) {
        auto symbol = st.lookup(std::make_pair(proc.getName(), decl->getName()),
                                decl->getLocation());
        auto index = symbol->getFrame()->getSize() - 1 + symbol->getStackOffset();
        outs << boost::format("    %s at index %d\n") % symbol->getName() % index;
      }
    }
    outs << "\n";
  }
public:
  ReportMemoryInfo(SymbolTable &symbolTable,
                   const std::vector<std::unique_ptr<hexasm::Directive>> &directives,
                   std::ostream &outs) :
    AstVisitor(false, false, false), st(symbolTable), directives(directives), outs(outs) {}
  void visitPre(Program &program) {
    auto stackPointer = dynamic_cast<hexasm::Data*>(directives[1].get())->getValue();
    outs << boost::format("Memory range 0x%x - 0x%x\n") % 0 % MAX_ADDRESS;
    outs << boost::format("Stack pointer initialised to 0x%x\n") % stackPointer;
    outs << boost::format("Arrays allocated 0x%x - 0x%x\n") % (stackPointer+1) % MAX_ADDRESS;
    outs << "\n";
  }
  void visitPre(Proc &proc) {
    auto procSymbol = st.lookup(std::make_pair(getCurrentScope(), proc.getName()),
                                proc.getLocation());
    reportFrame(procSymbol->getFrame(), proc);
  }
};

//===---------------------------------------------------------------------===//
// Driver.
//===---------------------------------------------------------------------===//

enum class DriverAction {
  EMIT_TOKENS,
  EMIT_TREE,
  EMIT_OPTIMISED_TREE,
  EMIT_INTERMEDIATE_INSTS,
  EMIT_LOWERED_INSTS,
  EMIT_ASM,
  EMIT_BINARY
};

class Driver {
  Lexer lexer;
  Parser parser;
  std::ostream &outStream;

public:
  Driver(std::ostream &outStream) :
    parser(lexer), outStream(outStream) {}

  int run(DriverAction action,
          const std::string &input,
          bool inputIsFilename,
          const std::string outputBinaryFilename="a.out",
          bool reportMemoryInfo=false) {

    // Open the file.
    if (inputIsFilename) {
      lexer.openFile(input);
    } else {
      lexer.loadBuffer(input);
    }

    // Tokenise only.
    if (action == DriverAction::EMIT_TOKENS && action != DriverAction::EMIT_TREE) {
      lexer.emitTokens(outStream);
      return 0;
    }

    // Parse the program.
    auto tree = parser.parseProgram();

    SymbolTable symbolTable;

    // Populate the symbol table;
    CreateSymbols createSymbols(symbolTable);
    tree->accept(&createSymbols);

    // Constant propagation.
    ConstProp constProp(symbolTable);
    tree->accept(&constProp);

    // Parse and print program only.
    if (action == DriverAction::EMIT_TREE) {
      xcmp::AstPrinter printer(outStream);
      tree->accept(&printer);
      return 0;
    }

    // Optimise expressions.
    OptimiseExpr optimiseExpr;
    tree->accept(&optimiseExpr);

    // Parse and print program only.
    if (action == DriverAction::EMIT_OPTIMISED_TREE) {
      xcmp::AstPrinter printer(outStream);
      tree->accept(&printer);
      return 0;
    }

    // Perform code generation.
    CodeGen codeGen(symbolTable);
    tree->accept(&codeGen);

    // Emit the generated intermediate instructions only.
    if (action == DriverAction::EMIT_INTERMEDIATE_INSTS) {
      codeGen.emitInstrs(outStream);
      return 0;
    }

    // Lower the generated (intermediate code) to assembly directives.
    xcmp::LowerDirectives lowerDirectives(symbolTable, codeGen);

    // Report frame information.
    if (reportMemoryInfo) {
      xcmp::ReportMemoryInfo reportMemoryInfo(symbolTable, lowerDirectives.getInstrs(), std::cout);
      tree->accept(&reportMemoryInfo);
    }

    // Emit the lowered instructions only.
    if (action == DriverAction::EMIT_LOWERED_INSTS) {
      lowerDirectives.emitInstrs(outStream);
      return 0;
    }

    // Assemble the instructions.
    hexasm::CodeGen asmCodeGen(lowerDirectives.getInstrs());

    // Print the assembly instructions only.
    if (action == DriverAction::EMIT_ASM) {
      asmCodeGen.emitProgramText(outStream);
      return 0;
    }

    if (action == DriverAction::EMIT_BINARY) {
      asmCodeGen.emitBin(outputBinaryFilename);
      return 0;
    }

    // No action.
    return 1;
  }

  int runCatchExceptions(DriverAction action,
                         const std::string &input,
                         bool inputIsFilename,
                         const std::string outputBinaryFilename="a.out",
                         bool reportMemoryInfo=false) {
    try {
      return run(action, input, inputIsFilename, outputBinaryFilename, reportMemoryInfo);
    } catch (const hexutil::Error &e) {
      if (e.hasLocation()) {
        std::cerr << boost::format("Error %s: %s\n") % e.getLocation().str() % e.what();
      } else {
        std::cerr << boost::format("Error: %s\n") % e.what();
      }
      if (lexer.hasLine()) {
        std::cerr << "  " << lexer.getLine() << "\n";
      }
      return 1;
    }
  }

  Lexer &getLexer() { return lexer; }
  Parser &getParser() { return parser; }
};

} // End namespace xcmp.

#endif // X_CMP_HPP
