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
//                 | "LDAI" | "LDBI | "STAI" | "BR" | "BZ" | "BN" | "BRB"
//                 | "SVC" | "ADD" | "SUB
// identifier     := <alpha> { <aplha> | <digit> | '_' }
// alpha          := 'a' | 'b' | ... | 'x' | 'A' | 'B' | ... | 'X'
// digit-not-zero := '1' | '2' | ... | '9'
// digit          := '0' | <digit-not-zero>
// natural-number := <digit-not-zero> { <digit> }
// integer-number := '0' | [ '-' ] <natural-number>

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
  BZ,
  BN,
  BRB,
  SVC,
  ADD,
  SUB,
  IDENTIFIER,
  UNKNOWN,
  END_OF_FILE
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

  Table         &table;
  std::ifstream &file;
  char          lastChar;
  std::string   identifier;
  unsigned      value;
  Token         lastToken;

  void declareKeywords() {
    table.insert("ADD",  Token::ADD);
    table.insert("BN",   Token::BN);
    table.insert("BR",   Token::BR);
    table.insert("BRB",  Token::BRB);
    table.insert("BZ",   Token::BZ);
    table.insert("DATA", Token::DATA);
    table.insert("FUNC", Token::FUNC);
    table.insert("LDAC", Token::LDAC);
    table.insert("LDAI", Token::LDAI);
    table.insert("LDAM", Token::LDAM);
    table.insert("LDAP", Token::LDAP);
    table.insert("LDBC", Token::LDBC);
    table.insert("LDBI", Token::LDBI);
    table.insert("LDBM", Token::LDBM);
    table.insert("PROC", Token::PROC);
    table.insert("STAI", Token::STAI);
    table.insert("STAM", Token::STAM);
    table.insert("SUB",  Token::SUB);
    table.insert("SVC",  Token::SVC);
  }

  int readChar() {
    file.get(lastChar);
    if (file.eof()) {
      lastChar = EOF;
    }
    return lastChar;
  }

  Token readToken() {
    // Skip whitespace.
    while (std::isspace(lastChar)) {
      readChar();
    }
    // Identifier.
    if (std::isalpha(lastChar)) {
      identifier = std::string(1, lastChar);
      while (std::isalnum(readChar())) {
        identifier += lastChar;
      }
      return table.lookup(identifier);
    }
    // Number
    if (std::isdigit(lastChar)) {
      std::string number(1, lastChar);
      while (std::isdigit(readChar())) {
        number += lastChar;
      }
      value = std::strtoul(number.c_str(), nullptr, 10);
      return Token::NUMBER;
    }
    // End of file.
    if (lastChar == EOF) {
      file.close();
      return Token::END_OF_FILE;
    }
    readChar();
    return Token::UNKNOWN;
  }

public:

  Lexer(Table &table, std::ifstream &file) :
      table(table), file(file) {
    declareKeywords();
    readChar();
  }

  Token getNextToken() {
    return lastToken = readToken();
  }

  const std::string &getIdentifier() const { return identifier; }
  unsigned getNumber() const { return value; }
  Token getLastToken() const { return lastToken; }
};

//===---------------------------------------------------------------------===//
// Parser
//===---------------------------------------------------------------------===//

// Base class for all directives.
struct Directive {
  virtual ~Directive() = default;
};

class Data : public Directive {
  int value;
public:
  Data(int value) : value(value) {}
};

class Func : public Directive {
  std::string identifier;
public:
  Func(std::string identifier) : identifier(identifier) {}
};

class Proc : public Directive {
  std::string identifier;
public:
  Proc(std::string identifier) : identifier(identifier) {}
};

class Label : public Directive {
  std::string label;
public:
  Label(std::string label) : label(label) {}
};

class Instruction : public Directive {
  Token opcode;
public:
  Instruction(Token opcode) : opcode(opcode) {}
};

class Parser {
  Lexer &lexer;

public:
  Parser(Lexer &lexer) : lexer(lexer) {}

  int parseInteger() {
    return -1;
  }

  std::string parseIdentifier() {
    return "";
  }

  std::unique_ptr<Directive> parseDirective() {
    switch (lexer.getLastToken()) {
      case Token::DATA:
        return std::make_unique<Data>(parseInteger());
      case Token::FUNC:
        return std::make_unique<Func>(parseIdentifier());
      case Token::PROC:
        return std::make_unique<Proc>(parseIdentifier());
      default:
        return nullptr;
    }
  }

  void parseProgram() {
    std::vector<std::unique_ptr<Directive>> program;
    while (lexer.getNextToken() != Token::END_OF_FILE) {
      program.push_back(parseDirective());
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
  try {
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

    Table table;
    std::ifstream file(filename);
    Lexer lexer(table, file);

    if (tokensOnly && !treeOnly) {
      while (true) {
        switch (lexer.getNextToken()) {
          case Token::LDAM: std::cout << "LDAM\n"; break;
          case Token::LDBM: std::cout << "LDAM\n"; break;
          case Token::STAM: std::cout << "LDAM\n"; break;
          case Token::LDAC: std::cout << "LDAM\n"; break;
          case Token::LDBC: std::cout << "LDAM\n"; break;
          case Token::LDAP: std::cout << "LDAM\n"; break;
          case Token::LDAI: std::cout << "LDAM\n"; break;
          case Token::LDBI: std::cout << "LDAM\n"; break;
          case Token::STAI: std::cout << "LDAM\n"; break;
          case Token::BR: std::cout << "LDAM\n"; break;
          case Token::BZ: std::cout << "LDAM\n"; break;
          case Token::BN: std::cout << "LDAM\n"; break;
          case Token::BRB: std::cout << "LDAM\n"; break;
          case Token::SVC: std::cout << "LDAM\n"; break;
          case Token::ADD: std::cout << "LDAM\n"; break;
          case Token::SUB: std::cout << "LDAM\n"; break;
          case Token::IDENTIFIER: std::cout << "IDENTIFIER " << lexer.getIdentifier() << "\n"; break;
          case Token::MINUS: std::cout << "MINUS\n"; break;
          case Token::NUMBER: std::cout << "NUMBER " << lexer.getNumber() << "\n"; break;
          case Token::FUNC: std::cout << "FUNC\n"; break;
          case Token::PROC: std::cout << "PROC\n"; break;
          case Token::UNKNOWN: std::cout << "UNKNOWN\n"; break;
          case Token::END_OF_FILE:
            std::cout << "EOF\n";
            std::exit(0);
        }
      }
      return 0;
    }

    if (treeOnly) {
      Parser parser(lexer);
      parser.parseProgram();
    }

  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    std::exit(1);
  }
  return 0;
}
