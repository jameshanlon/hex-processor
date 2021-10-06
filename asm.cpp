#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>

// With inspiration from the LLVM Kaleidoscope tutorial.
// https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html

// EBNF grammar:
//
// program        := { <label> | <data> | <instruction> | <func> | <proc> }
// label          := <alpha> <natural-number>
// data           := <data> <integer-number>
// func           := "FUNC" <identifier>
// proc           := "PROC" <identifier>
// instruction    := <opcode> <number>
//                 | <opcode> <label>
//                 | "OPR" <opcode>
// operand        := <number>
//                 | <label>
// opcode         := "LDAM" | "LDBM" | "STAM" | "LDAC" | "LDBC" | "LDAP"
//                 | "LDAI" | "LDBI | "STAI" | "BR" | "BRZ" | "BRN" | "BRB"
//                 | "SVC" | "ADD" | "SUB
// identifier     := <alpha> { <aplha> | <digit> | '_' }
// alpha          := 'a' | 'b' | ... | 'x' | 'A' | 'B' | ... | 'X'
// digit-not-zero := '1' | '2' | ... | '9'
// digit          := '0' | <digit-not-zero>
// natural-number := <digit-not-zero> { <digit> }
// integer-number := '0' | [ '-' ] <natural-number>
//
// Comments start with '#' and continue to the end of the line.

//===---------------------------------------------------------------------===//
// Lexer
//===---------------------------------------------------------------------===//

enum class Token {
  NUMBER,
  MINUS,
  DATA,
  PROC,
  FUNC,
  LDAM,
  LDBM,
  STAM,
  LDAC,
  LDBC,
  LDAP,
  LDAI,
  LDBI,
  STAI,
  BR,
  BRZ,
  BRN,
  BRB,
  SVC,
  ADD,
  SUB,
  OPR,
  IDENTIFIER,
  NONE,
  END_OF_FILE
};

const char *tokenEnumStr(Token token) {
  switch (token) {
  case Token::NUMBER:      return "NUMBER";
  case Token::MINUS:       return "MINUS";
  case Token::DATA:        return "DATA";
  case Token::PROC:        return "PROC";
  case Token::FUNC:        return "FUNC";
  case Token::LDAM:        return "LDAM";
  case Token::LDBM:        return "LDBM";
  case Token::STAM:        return "STAM";
  case Token::LDAC:        return "LDAC";
  case Token::LDBC:        return "LDBC";
  case Token::LDAP:        return "LDAP";
  case Token::LDAI:        return "LDAI";
  case Token::LDBI:        return "LDBI";
  case Token::STAI:        return "STAI";
  case Token::BR:          return "BR";
  case Token::BRZ:         return "BRZ";
  case Token::BRN:         return "BRN";
  case Token::BRB:         return "BRB";
  case Token::SVC:         return "SVC";
  case Token::ADD:         return "ADD";
  case Token::SUB:         return "SUB";
  case Token::OPR:         return "OPR";
  case Token::IDENTIFIER:  return "IDENTIFIER";
  case Token::NONE:        return "NONE";
  case Token::END_OF_FILE: return "END_OF_FILE";
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
  unsigned      value;
  Token         lastToken;
  size_t        currentLine;

  void declareKeywords() {
    table.insert("ADD",  Token::ADD);
    table.insert("BRN",  Token::BRN);
    table.insert("BR",   Token::BR);
    table.insert("BRB",  Token::BRB);
    table.insert("BRZ",  Token::BRZ);
    table.insert("DATA", Token::DATA);
    table.insert("FUNC", Token::FUNC);
    table.insert("LDAC", Token::LDAC);
    table.insert("LDAI", Token::LDAI);
    table.insert("LDAM", Token::LDAM);
    table.insert("LDAP", Token::LDAP);
    table.insert("LDBC", Token::LDBC);
    table.insert("LDBI", Token::LDBI);
    table.insert("LDBM", Token::LDBM);
    table.insert("OPR",  Token::OPR);
    table.insert("PROC", Token::PROC);
    table.insert("STAI", Token::STAI);
    table.insert("STAM", Token::STAM);
    table.insert("SUB",  Token::SUB);
    table.insert("SVC",  Token::SVC);
  }

  int readChar() {
    file.get(lastChar);
    std::cout << lastChar;
    if (file.eof()) {
      lastChar = EOF;
    }
    return lastChar;
  }

  Token readToken() {
    // Skip whitespace.
    while (std::isspace(lastChar)) {
      if (lastChar == '\n') {
        currentLine++;
      }
      readChar();
    }
    // Comment.
    if (lastChar == '#') {
      do {
        readChar();
      } while (lastChar != EOF && lastChar != '\n');
      if (lastChar == '\n') {
        currentLine++;
      }
      return readToken();
    }
    // Identifier.
    if (std::isalpha(lastChar)) {
      identifier = std::string(1, lastChar);
      while (std::isalnum(readChar())) {
        identifier += lastChar;
      }
      return table.lookup(identifier);
    }
    // Number.
    if (std::isdigit(lastChar)) {
      std::string number(1, lastChar);
      while (std::isdigit(readChar())) {
        number += lastChar;
      }
      value = std::strtoul(number.c_str(), nullptr, 10);
      return Token::NUMBER;
    }
    // Symbols.
    if (lastChar == '-') {
      readChar();
      return Token::MINUS;
    }
    // End of file.
    if (lastChar == EOF) {
      file.close();
      return Token::END_OF_FILE;
    }
    readChar();
    return Token::NONE;
  }

public:

  Lexer() : currentLine(0) {
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
  size_t getLine() const { return currentLine; }
};

//===---------------------------------------------------------------------===//
// Directive data types.
//===---------------------------------------------------------------------===//

// Base class for all directives.
struct Directive {
  virtual ~Directive() = default;
  virtual std::string toString() const = 0;
};

class Data : public Directive {
  int value;
public:
  Data(int value) : value(value) {}
  std::string toString() const {
    return "DATA " + std::to_string(value);
  }
};

class Func : public Directive {
  std::string identifier;
public:
  Func(std::string identifier) : identifier(identifier) {}
  std::string toString() const {
    return "FUNC " + identifier;
  }
};

class Proc : public Directive {
  std::string identifier;
public:
  Proc(std::string identifier) : identifier(identifier) {}
  std::string toString() const {
    return "PROC " + identifier;
  }
};

class Label : public Directive {
  std::string label;
public:
  Label(std::string label) : label(label) {}
  std::string toString() const {
    return label;
  }
};

class InstrImm : public Directive {
  Token opcode;
  int value;
public:
  InstrImm(Token opcode, int value) : opcode(opcode), value(value) {}
  std::string toString() const {
    return std::string(tokenEnumStr(opcode)) + " " + std::to_string(value);
  }
};

class InstrLabel : public Directive {
  Token opcode;
  std::string label;
public:
  InstrLabel(Token opcode, std::string label) : opcode(opcode), label(label) {}
  std::string toString() const {
    return std::string(tokenEnumStr(opcode)) + " " + label;
  }
};

class OPR : public Directive {
  Token opcode;
public:
  OPR(Token opcode) : opcode(opcode) {
    if (opcode != Token::BRB &&
        opcode != Token::ADD &&
        opcode != Token::SUB &&
        opcode != Token::SVC) {
      throw std::runtime_error(std::string("unexpected operand to OPR ")+tokenEnumStr(opcode));
    }
  }
  std::string toString() const {
    return std::string("OPR ") + tokenEnumStr(opcode);
  }
};

//===---------------------------------------------------------------------===//
// Parser
//===---------------------------------------------------------------------===//

class Parser {
  Lexer &lexer;
  std::vector<std::unique_ptr<Directive>> program;

  void expectLast(Token token) const {
    if (token != lexer.getLastToken()) {
      throw std::runtime_error(std::string("expected ")+tokenEnumStr(token));
    }
  }

  void expectNext(Token token) const {
    lexer.getNextToken();
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

  std::string parseIdentifier() {
    lexer.getNextToken();
    return lexer.getIdentifier();
  }

  std::unique_ptr<Directive> parseDirective() {
    switch (lexer.getLastToken()) {
      case Token::DATA:
        lexer.getNextToken();
        return std::make_unique<Data>(parseInteger());
      case Token::FUNC:
        lexer.getNextToken();
        return std::make_unique<Func>(parseIdentifier());
      case Token::PROC:
        lexer.getNextToken();
        return std::make_unique<Proc>(parseIdentifier());
      case Token::IDENTIFIER:
        return std::make_unique<Label>(lexer.getIdentifier());
      case Token::OPR:
        return std::make_unique<OPR>(lexer.getNextToken());
      case Token::LDAM:
      case Token::LDBM:
      case Token::STAM:
      case Token::LDAC:
      case Token::LDBC:
      case Token::LDAI:
      case Token::LDBI:
      case Token::STAI:
      case Token::LDAP:
      case Token::BRN:
      case Token::BR:
      case Token::BRZ: {
        auto opcode = lexer.getLastToken();
        if (lexer.getNextToken() == Token::IDENTIFIER) {
          return std::make_unique<InstrLabel>(opcode, lexer.getIdentifier());
        } else {
          return std::make_unique<InstrImm>(opcode, parseInteger());
        }
      }
      default:
        throw std::runtime_error("unrecognised token");
    }
  }

public:
  Parser(Lexer &lexer) : lexer(lexer) {}

  void parseProgram() {
    while (lexer.getNextToken() != Token::END_OF_FILE) {
      program.push_back(parseDirective());
    }
  }

  void printProgram() const {
    for (auto &directive : program) {
      std::cout << directive->toString() + '\n';
    }
  }
};

//===---------------------------------------------------------------------===//
// Driver
//===---------------------------------------------------------------------===//

static void help(const char *argv[]) {
  std::cout << "Hex assembler\n\n";
  std::cout << "Usage: " << argv[0] << " file\n\n";
  std::cout << "Positional arguments:\n";
  std::cout << "  file        A source file to assemble\n\n";
  std::cout << "Optional arguments:\n";
  std::cout << "  -h,--help   Display this message\n";
  std::cout << "  --tokens    Tokenise the input only\n";
  std::cout << "  --tree      Display the syntax tree only\n";
}

int main(int argc, const char *argv[]) {
  Lexer lexer;
  Parser parser(lexer);

  try {

    // Handle arguments.
    bool tokensOnly = false;
    bool treeOnly = false;
    const char *filename = nullptr;
    for (unsigned i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0) {
        help(argv);
        std::exit(1);
      } else if (std::strcmp(argv[i], "--tokens") == 0) {
        tokensOnly = true;
      } else if (std::strcmp(argv[i], "--tree") == 0) {
        treeOnly = true;
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
          case Token::LDAM:
          case Token::LDBM:
          case Token::STAM:
          case Token::LDAC:
          case Token::LDBC:
          case Token::LDAP:
          case Token::LDAI:
          case Token::LDBI:
          case Token::STAI:
          case Token::BR:
          case Token::BRZ:
          case Token::BRN:
          case Token::BRB:
          case Token::SVC:
          case Token::ADD:
          case Token::SUB:
          case Token::MINUS:
          case Token::FUNC:
          case Token::PROC:
          case Token::DATA:
          case Token::OPR:
          case Token::NONE:
            std::cout << tokenEnumStr(lexer.getLastToken()) << "\n";
            break;
          case Token::IDENTIFIER:
            std::cout << "IDENTIFIER " << lexer.getIdentifier() << "\n";
            break;
          case Token::NUMBER:
            std::cout << "NUMBER " << lexer.getNumber() << "\n";
            break;
          case Token::END_OF_FILE:
            std::cout << "EOF\n";
            std::exit(0);
        }
      }
      return 0;
    }

    // Parse the program.
    parser.parseProgram();

    // Parse and print program only.
    if (treeOnly) {
      parser.printProgram();
      return 0;
    }

    // Expand immediates.
    // Patch labels.

  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << " : " << lexer.getLine() << "\n";
    std::exit(1);
  }
  return 0;
}
