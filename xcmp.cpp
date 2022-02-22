#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <boost/format.hpp>

#include "hex.hpp"
#include "hexasm.hpp"

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
      readChar();
      break;
    case '\"':
      readChar();
      readString();
      token = Token::STRING;
      if (lastChar != '"') {
        throw std::runtime_error("expected \" after string");
      }
      readChar();
      break;
    case EOF:
      file.close();
      token = Token::END_OF_FILE;
      readChar();
      break;
    default:
      throw std::runtime_error("unexpected character");
    }
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
public:
  virtual ~AstNode() = default;
  virtual void accept(AstVisitor* visitor) = 0;
};

// Expressions ============================================================= //

class Expr : public AstNode {};

class VarRefExpr : public Expr {
  std::string name;
  std::unique_ptr<Expr> expr;
public:
  VarRefExpr(std::string name) : name(name) {}
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
  ArraySubscriptExpr(std::string name, std::unique_ptr<Expr> expr) :
      name(name), expr(std::move(expr)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    expr->accept(visitor);
    visitor->visitPost(*this);
  }
};

class CallExpr : public Expr {
  std::string name;
  std::vector<std::unique_ptr<Expr>> args;
public:
  CallExpr(std::string name) : name(name) {}
  CallExpr(std::string name, std::vector<std::unique_ptr<Expr>> args) :
      name(name), args(std::move(args)) {}
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
  NumberExpr(unsigned value) : value(value) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
  unsigned getValue() const { return value; }
};

class BooleanExpr : public Expr {
  bool value;
public:
  BooleanExpr(bool value) : value(value) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
  bool getValue() const { return value; }
};

class StringExpr : public Expr {
  std::string value;
public:
  StringExpr(std::string value) : value(value) {}
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
  UnaryOpExpr(Token op, std::unique_ptr<Expr> element) :
      op(op), element(std::move(element)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    element->accept(visitor);
    visitor->visitPost(*this);
  }
  Token getOp() const { return op; }
};

class BinaryOpExpr : public Expr {
  Token op;
  std::unique_ptr<Expr> LHS, RHS;
public:
  BinaryOpExpr(Token op, std::unique_ptr<Expr> LHS, std::unique_ptr<Expr> RHS) :
      op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    LHS->accept(visitor);
    RHS->accept(visitor);
    visitor->visitPost(*this);
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
    visitor->visitPre(*this);
    expr->accept(visitor);
    visitor->visitPost(*this);
  }
};

class VarDecl : public Decl {
public:
  VarDecl(std::string name) : Decl(name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

class ArrayDecl : public Decl {
  std::unique_ptr<Expr> expr;
public:
  ArrayDecl(std::string name, std::unique_ptr<Expr> expr) :
      Decl(name), expr(std::move(expr)) {}
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
  Formal(std::string name) : name(name) {}
  std::string getName() const { return name; }
};

class ValFormal : public Formal {
public:
  ValFormal(std::string name) : Formal(name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

class ArrayFormal : public Formal {
public:
  ArrayFormal(std::string name) : Formal(name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

class ProcFormal : public Formal {
public:
  ProcFormal(std::string name) : Formal(name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

class FuncFormal : public Formal {
public:
  FuncFormal(std::string name) : Formal(name) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

// Statement ================================================================ //

class Statement : public AstNode {};

class SkipStatement : public Statement {
public:
  SkipStatement() {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

class StopStatement : public Statement {
public:
  StopStatement() {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    visitor->visitPost(*this);
  }
};

class ReturnStatement : public Statement {
  std::unique_ptr<Expr> expr;
public:
  ReturnStatement(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}
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
  IfStatement(std::unique_ptr<Expr> condition,
              std::unique_ptr<Statement> thenStmt,
              std::unique_ptr<Statement> elseStmt) :
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
  WhileStatement(std::unique_ptr<Expr> condition,
                 std::unique_ptr<Statement> stmt) :
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
  SeqStatement(std::vector<std::unique_ptr<Statement>> stmts) :
      stmts(std::move(stmts)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    for (auto &stmt : stmts) {
      stmt->accept(visitor);
    }
    visitor->visitPost(*this);
  }
};

class CallStatement : public Statement {
  std::unique_ptr<Expr> call;
  // TODO: work out a better way than dynamically casting.
public:
  CallStatement(std::unique_ptr<Expr> call) :
      call(std::move(call)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    for (auto &arg : dynamic_cast<CallExpr*>(call.get())->getArgs()) {
      arg->accept(visitor);
    }
    visitor->visitPost(*this);
  }
  const std::string &getName() const { return dynamic_cast<CallExpr*>(call.get())->getName(); }
};

class AssStatement : public Statement {
  std::unique_ptr<Expr> LHS, RHS;
public:
  AssStatement(std::unique_ptr<Expr> LHS,
               std::unique_ptr<Expr> RHS) :
      LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  virtual void accept(AstVisitor *visitor) override {
    visitor->visitPre(*this);
    LHS->accept(visitor);
    RHS->accept(visitor);
    visitor->visitPost(*this);
  }
};

// Procedures and functions ================================================= //

class Proc : public AstNode {
  std::string name;
  std::vector<std::unique_ptr<Formal>> formals;
  std::vector<std::unique_ptr<Decl>> decls;
  std::unique_ptr<Statement> statement;
public:
  Proc(std::string name,
       std::vector<std::unique_ptr<Formal>> formals,
       std::vector<std::unique_ptr<Decl>> decls,
       std::unique_ptr<Statement> statement) :
      name(name), formals(std::move(formals)),
      decls(std::move(decls)), statement(std::move(statement)) {}
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
  std::string getName() const { return name; }
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
    indent(); outs << boost::format("proc %s\n") % decl.getName();
    indentCount++;
  };
  void visitPost(Proc &decl) override {
    indentCount--;
  }
  void visitPre(ArrayDecl &decl) override {
    indent(); outs << boost::format("arraydecl %s\n") % decl.getName();
    indentCount++;
  };
  void visitPost(ArrayDecl &decl) override {
    indentCount--;
  }
  void visitPre(VarDecl &decl) override {
    indent(); outs << boost::format("vardecl %s\n") % decl.getName();
  };
  void visitPost(VarDecl &decl) override { }
  void visitPre(ValDecl &decl) override {
    indent(); outs << boost::format("valdecl %s\n") % decl.getName();
    indentCount++;
  };
  void visitPost(ValDecl &decl) override {
    indentCount--;
  }
  void visitPre(BinaryOpExpr &expr) override {
    indent(); outs << boost::format("binaryop %s\n") % tokenEnumStr(expr.getOp());
    indentCount++;
  };
  void visitPost(BinaryOpExpr &expr) override {
    indentCount--;
  }
  void visitPre(UnaryOpExpr &expr) override {
    indent(); outs << boost::format("unaryop %s\n") % tokenEnumStr(expr.getOp());
    indentCount++;
  };
  void visitPost(UnaryOpExpr &expr) override {
    indentCount--;
  }
  void visitPre(StringExpr &expr) override {
    indent(); outs << "unaryop\n";
  };
  void visitPost(StringExpr &expr) override { }
  void visitPre(BooleanExpr &expr) override {
    indent(); outs << "boolean\n";
  };
  void visitPost(BooleanExpr &expr) override { }
  void visitPre(NumberExpr &expr) override {
    indent(); outs << boost::format("number %d\n") % expr.getValue();
  };
  void visitPost(NumberExpr &expr) override { }
  void visitPre(CallExpr &expr) override {
    indent(); outs << boost::format("call %d\n") % expr.getName();
    indentCount++;
  };
  void visitPost(CallExpr &expr) override {
    indentCount--;
  }
  void visitPre(ArraySubscriptExpr &expr) override {
    indent(); outs << "arraysubscript\n";
    indentCount++;
  };
  void visitPost(ArraySubscriptExpr &expr) override {
    indentCount--;
  }
  void visitPre(VarRefExpr &expr) override {
    indent(); outs << boost::format("varref %s\n") % expr.getName();
  };
  void visitPost(VarRefExpr &expr) override { }
  void visitPre(ValFormal &formal) override {
    indent(); outs << boost::format("valformal %s\n") % formal.getName();
  };
  void visitPost(ValFormal &formal) override {};
  void visitPre(ArrayFormal &formal) override {
    indent(); outs << boost::format("arrayformal %s\n") % formal.getName();
  };
  void visitPost(ArrayFormal &formal) override {};
  void visitPre(ProcFormal &formal) override {
    indent(); outs << boost::format("procformal %s\n") % formal.getName();
  };
  void visitPost(ProcFormal &formal) override {};
  void visitPre(FuncFormal &formal) override {
    indent(); outs << boost::format("funcformal %s\n") % formal.getName();
  };
  void visitPost(FuncFormal &formal) override {};
  void visitPre(SkipStatement &stmt) override {
    indent(); outs << "skipstmt\n";
  };
  void visitPost(SkipStatement &stmt) override {};
  void visitPre(StopStatement &stmt) override {
    indent(); outs << "stopstmt\n";
  };
  void visitPost(StopStatement &stmt) override {};
  void visitPre(ReturnStatement &stmt) override {
    indent(); outs << "returnstmt\n";
    indentCount++;
  };
  void visitPost(ReturnStatement &stmt) override {
    indentCount--;
  };
  void visitPre(IfStatement &stmt) override {
    indent(); outs << "ifstmt\n";
    indentCount++;
  };
  void visitPost(IfStatement &stmt) override {
    indentCount--;
  };
  void visitPre(WhileStatement &stmt) override {
    indent(); outs << "whilestmt\n";
    indentCount++;
  };
  void visitPost(WhileStatement &stmt) override {
    indentCount--;
  };
  void visitPre(SeqStatement &stmt) override {
    indent(); outs << "seqstmt\n";
    indentCount++;
  };
  void visitPost(SeqStatement &stmt) override {
    indentCount--;
  };
  void visitPre(CallStatement &stmt) override {
    indent(); outs << boost::format("callstmt %s\n") % stmt.getName();
    indentCount++;
  };
  void visitPost(CallStatement &stmt) override {
    indentCount--;
  };
  void visitPre(AssStatement &stmt) override {
    indent(); outs << "assstmt\n";
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
      throw std::runtime_error(std::string("expected ")+tokenEnumStr(token));
    }
    lexer.getNextToken();
  }

  int parseInteger() {
    if (lexer.getLastToken() == Token::MINUS) {
       lexer.getNextToken();
       expect(Token::NUMBER);
       return -lexer.getNumber();
    }
    expect(Token::NUMBER);
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
      lexer.getNextToken();
      auto RHS = parseBinOpRHS(op);
      return std::make_unique<BinaryOpExpr>(op, std::move(element), std::move(RHS));
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
      lexer.getNextToken();
      auto element = parseElement();
      return std::make_unique<UnaryOpExpr>(Token::MINUS, std::move(element));
    }
    if (lexer.getLastToken() == Token::NOT) {
      lexer.getNextToken();
      auto element = parseElement();
      return std::make_unique<UnaryOpExpr>(Token::NOT, std::move(element));
    }
    auto element = parseElement();
    if (isBinaryOp(lexer.getLastToken())) {
      // Binary operation.
      auto op = lexer.getLastToken();
      lexer.getNextToken();
      auto RHS = parseBinOpRHS(op);
      return std::make_unique<BinaryOpExpr>(op, std::move(element), std::move(RHS));
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
    switch (lexer.getLastToken()) {
    case Token::IDENTIFIER: {
      auto name = parseIdentifier();
      // Array subscript.
      if (lexer.getLastToken() == Token::LBRACKET) {
        lexer.getNextToken();
        auto expr = parseExpr();
        expect(Token::RBRACKET);
        return std::make_unique<ArraySubscriptExpr>(name, std::move(expr));
      // Procedure call.
      } else if (lexer.getLastToken() == Token::LPAREN) {
        if (lexer.getNextToken() == Token::RPAREN) {
          lexer.getNextToken();
          return std::make_unique<CallExpr>(name);
        } else {
          auto exprList = parseExprList();
          expect(Token::RPAREN);
          return std::make_unique<CallExpr>(name, std::move(exprList));
        }
      // Variable reference.
      } else {
        return std::make_unique<VarRefExpr>(name);
      }
    }
    case Token::NUMBER:
      lexer.getNextToken();
      return std::make_unique<NumberExpr>(lexer.getNumber());
    case Token::STRING:
      lexer.getNextToken();
      return std::make_unique<StringExpr>(lexer.getString());
    case Token::TRUE:
      lexer.getNextToken();
      return std::make_unique<BooleanExpr>(true);
    case Token::FALSE:
      lexer.getNextToken();
      return std::make_unique<BooleanExpr>(false);
    case Token::LPAREN: {
      lexer.getNextToken();
      auto expr = parseExpr();
      expect(Token::RPAREN);
      return expr;
    }
    default:
      std::cout << tokenEnumStr(lexer.getLastToken()) << "\n";
      throw std::runtime_error("in expression element");
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
      expect(Token::EQ);
      auto expr = parseExpr();
      expect(Token::SEMICOLON);
      return std::make_unique<ValDecl>(name, std::move(expr));
    }
    case Token::VAR: {
      lexer.getNextToken();
      auto name = parseIdentifier();
      expect(Token::SEMICOLON);
      return std::make_unique<VarDecl>(name);
    }
    case Token::ARRAY: {
      lexer.getNextToken();
      auto name = parseIdentifier();
      expect(Token::LBRACKET);
      auto expr = parseExpr();
      expect(Token::RBRACKET);
      expect(Token::SEMICOLON);
      return std::make_unique<ArrayDecl>(name, std::move(expr));
    }
    default:
      throw std::runtime_error("invalid declaration");
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
      while (lexer.getLastToken() == Token::VAL ||
             lexer.getLastToken() == Token::ARRAY ||
             lexer.getLastToken() == Token::PROC ||
             lexer.getLastToken() == Token::FUNC) {
      formals.push_back(parseFormal());
      if (lexer.getLastToken() == Token::COMMA) {
        lexer.getNextToken();
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
    switch (lexer.getLastToken()) {
    case Token::VAL:
      lexer.getNextToken();
      return std::make_unique<ValFormal>(parseIdentifier());
    case Token::ARRAY:
      lexer.getNextToken();
      return std::make_unique<ArrayFormal>(parseIdentifier());
    case Token::PROC:
      lexer.getNextToken();
      return std::make_unique<ProcFormal>(parseIdentifier());
    case Token::FUNC:
      lexer.getNextToken();
      return std::make_unique<FuncFormal>(parseIdentifier());
    default:
      throw std::runtime_error("invalid formal");
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
    switch (lexer.getLastToken()) {
    case Token::SKIP:
      lexer.getNextToken();
      return std::make_unique<SkipStatement>();
    case Token::STOP:
      lexer.getNextToken();
      return std::make_unique<StopStatement>();
    case Token::RETURN:
      lexer.getNextToken();
      return std::make_unique<ReturnStatement>(parseExpr());
    case Token::IF: {
      lexer.getNextToken();
      auto condition = parseExpr();
      expect(Token::THEN);
      auto thenStmt = parseStatement();
      expect(Token::ELSE);
      auto elseStmt = parseStatement();
      return std::make_unique<IfStatement>(std::move(condition),
                                           std::move(thenStmt),
                                           std::move(elseStmt));
    }
    case Token::WHILE: {
      lexer.getNextToken();
      auto condition = parseExpr();
      expect(Token::DO);
      auto stmt = parseStatement();
      return std::make_unique<WhileStatement>(std::move(condition), std::move(stmt));
    }
    case Token::BEGIN: {
      lexer.getNextToken();
      auto body = parseStatements();
      expect(Token::END);
      return std::make_unique<SeqStatement>(std::move(body));
    }
    case Token::IDENTIFIER: {
      auto element = parseElement();
      // Procedure call
      if (dynamic_cast<CallExpr*>(element.get())) {
        return std::make_unique<CallStatement>(std::move(element));
      }
      // Assignment
      expect(Token::ASS);
      return std::make_unique<AssStatement>(std::move(element), parseExpr());
    }
    default:
      throw std::runtime_error("invalid statement");
    }
  }

  /// proc-decl :=
  ///  "proc" <name> "(" <formals> ")" "is" [0 <var-decl> ] <statement>
  std::unique_ptr<Proc> parseProcDecl() {
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
    return std::make_unique<Proc>(name, std::move(formals),
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
// Code generation.
//===---------------------------------------------------------------------===//

class CodeGen : public AstVisitor {

  std::vector<std::unique_ptr<hexasm::Directive>> instrs;

  void genData(uint32_t value) {
    instrs.push_back(std::make_unique<hexasm::Data>(hexasm::Token::DATA, value));
  }

  void genLabel(std::string name) {
    instrs.push_back(std::make_unique<hexasm::Label>(hexasm::Token::IDENTIFIER, name));
  }

  void genBR(std::string label) {
    instrs.push_back(std::make_unique<hexasm::InstrLabel>(hexasm::Token::BR, label, true));
  }

  void genLDAC(int value) {
    instrs.push_back(std::make_unique<hexasm::InstrImm>(hexasm::Token::LDAC, value));
  }

  void genOPR(hexasm::Token op) {
    instrs.push_back(std::make_unique<hexasm::InstrOp>(hexasm::Token::OPR, op));
  }

public:
  CodeGen() {
    genBR("start");
    genData(1<<16);
    genLabel("start");
    genBR("exit");
    genLabel("exit");
    genLDAC(0);
    genOPR(hexasm::Token::SVC);
  }
  void visitPre(Program &tree) {}
  void visitPost(Program &tree) {}
  std::vector<std::unique_ptr<hexasm::Directive>> &getInstrs() {
    return instrs;
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
    auto tree = parser.parseProgram();

    // Parse and print program only.
    if (treeOnly) {
      AstPrinter printer;
      tree->accept(&printer);
      return 0;
    }

    // Generate code.
    CodeGen codeGen;
    tree->accept(&codeGen);
    auto programSize = hexasm::prepareProgram(codeGen.getInstrs());
    hexasm::emitBin(codeGen.getInstrs(), outputFilename, programSize);

  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    std::cerr << "  " << lexer.getLineNumber() << ": " << lexer.getLine() << "\n";
    std::exit(1);
  }
  return 0;
}
