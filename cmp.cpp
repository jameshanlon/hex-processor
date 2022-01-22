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
  case Token::SEMICOLON:   return ":";
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

  int readCharConst() {
    readChar();
    int value;
    if (lastChar == '\\') {
      readChar();
      switch (lastChar) {
      case '\\': value = '\\'; break;
      case '\'': value = '\''; break;
      case '"':  value = '"';  break;
      case 't':  value = '\t'; break;
      case 'r':  value = '\r'; break;
      case 'n':  value = '\n'; break;
      default: throw std::runtime_error("bad character constant");
      }
    } else {
      value = static_cast<int>(lastChar);
    }
    readChar();
    if (lastChar != '\'') {
      throw std::runtime_error("expected ' after char constant");
    }
    return value;
  }

  void readString() {
    string.clear();
    do {
      string += readCharConst();
    }
    while (lastChar != '"' && lastChar != EOF);
    readChar();
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
      value = readCharConst();
      token = Token::NUMBER;
      if (lastChar != '\'') {
        throw std::runtime_error("expected ' after char constant");
      }
      break;
    case '\"':
      readString();
      token = Token::STRING;
      if (lastChar != '"') {
        throw std::runtime_error("expected \" after string");
      }
      break;
    case EOF:
      file.close();
      token = Token::END_OF_FILE; break;
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
  unsigned getNumber() const { return value; }
  Token getLastToken() const { return lastToken; }
  size_t getLineNumber() const { return currentLineNumber; }
  const std::string &getLine() const { return currentLine; }
};

//===---------------------------------------------------------------------===//
// Parser
//===---------------------------------------------------------------------===//

//class Parser {
//  Lexer &lexer;
//
//  void expectLast(Token token) const {
//    if (token != lexer.getLastToken()) {
//      throw std::runtime_error(std::string("expected ")+tokenEnumStr(token));
//    }
//  }
//
//  void expectNext(Token token) const {
//    lexer.getNextToken();
//    expectLast(token);
//  }
//
//  int parseInteger() {
//    if (lexer.getLastToken() == Token::MINUS) {
//       expectNext(Token::NUMBER);
//       return -lexer.getNumber();
//    }
//    expectLast(Token::NUMBER);
//    return lexer.getNumber();
//  }
//
//  std::string parseIdentifier() {
//    lexer.getNextToken();
//    return lexer.getIdentifier();
//  }
//
//  std::unique_ptr<Directive> parseDirective() {
//    switch (lexer.getLastToken()) {
//      case Token::DATA:
//        lexer.getNextToken();
//        return std::make_unique<Data>(Token::DATA, parseInteger());
//      case Token::FUNC:
//        return std::make_unique<Func>(Token::FUNC, parseIdentifier());
//      case Token::PROC:
//        return std::make_unique<Proc>(Token::PROC, parseIdentifier());
//      case Token::IDENTIFIER:
//        return std::make_unique<Label>(Token::IDENTIFIER, lexer.getIdentifier());
//      case Token::OPR:
//        return std::make_unique<InstrOp>(Token::OPR, lexer.getNextToken());
//      case Token::LDAM:
//      case Token::LDBM:
//      case Token::STAM:
//      case Token::LDAC:
//      case Token::LDBC: {
//        auto opcode = lexer.getLastToken();
//        if (lexer.getNextToken() == Token::IDENTIFIER) {
//          return std::make_unique<InstrLabel>(opcode, lexer.getIdentifier(), false);
//        } else {
//          return std::make_unique<InstrImm>(opcode, parseInteger());
//        }
//      }
//      case Token::LDAP:
//      case Token::LDAI:
//      case Token::LDBI:
//      case Token::STAI:
//      case Token::BR:
//      case Token::BRN:
//      case Token::BRZ: {
//        auto opcode = lexer.getLastToken();
//        if (lexer.getNextToken() == Token::IDENTIFIER) {
//          return std::make_unique<InstrLabel>(opcode, lexer.getIdentifier(), true);
//        } else {
//          return std::make_unique<InstrImm>(opcode, parseInteger());\
//        }
//      }
//      default:
//        throw std::runtime_error(std::string("unrecognised token ")+tokenEnumStr(lexer.getLastToken()));
//    }
//  }
//
//public:
//  Parser(Lexer &lexer) : lexer(lexer) {}
//
//  std::vector<std::unique_ptr<Directive>> parseProgram() {
//    std::vector<std::unique_ptr<Directive>> program;
//    while (lexer.getNextToken() != Token::END_OF_FILE) {
//      program.push_back(parseDirective());
//    }
//    return program;
//  }
//};

///// Emit the program to stdout.
//static void emitProgramText(std::vector<std::unique_ptr<Directive>> &program) {
//  for (auto &directive : program) {
//    std::cout << boost::format("%#08x %-20s (%d bytes)\n")
//                   % directive->getByteOffset()
//                   % directive->toString()
//                   % directive->getSize();
//  }
//  std::cout << boost::format("%d bytes\n") % getProgramSize(program);
//}

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
  //Parser parser(lexer);

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
    //auto program = parser.parseProgram();

    // Parse and print program only.
    if (treeOnly) {
      //emitProgramText(program);
      return 0;
    }

  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    std::cerr << "  " << lexer.getLineNumber() << ": " << lexer.getLine() << "\n";
    std::exit(1);
  }
  return 0;
}
