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

//===---------------------------------------------------------------------===//
// Lexer
//===---------------------------------------------------------------------===//

enum class Token {
  OPCODE,
  NUMBER,
  PROC,
  NAME,
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
    table.insert(std::make_pair(name, Token::NAME));
    return Token::NAME;
  }
};

class Lexer {

  Table         &table;
  std::ifstream &file;
  size_t        lineIndex;
  size_t        columnIndex;
  char          lastChar;
  std::string   identifier;
  unsigned      value;
  Token         lastToken;

  void declareKeywords() {
    table.insert("PROC", Token::PROC);
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
      table(table), file(file), lineIndex(0), columnIndex(0) {
    declareKeywords();
    readChar();
  }

  Token getNextToken() {
    return lastToken = readToken();
  }

  const std::string &getName() const { return identifier; }
  unsigned getNumber() const { return value; }
};

static void help(const char *argv[]) {
  std::cout << "X assembler\n\n";
  std::cout << "Usage: " << argv[0] << " file\n\n";
  std::cout << "Optional arguments:\n";
  std::cout << "  -h,--help display this message\n";
}

int main(int argc, const char *argv[]) {

  try {
    const char *filename = nullptr;
    for (unsigned i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0) {
        help(argv);
        std::exit(1);
      } else {
        if (!filename) {
          filename = argv[i];
        } else {
          throw std::runtime_error("cannot specify more than one file");
        }
      }
    }

    if (!filename) {
      help(argv);
      std::exit(1);
    }

    Table table;
    std::ifstream file(filename);
    Lexer lexer(table, file);

    while (true) {
      switch (lexer.getNextToken()) {
        case Token::NAME:
          std::cout << "NAME " << lexer.getName() << "\n";
          break;
        case Token::NUMBER:
          std::cout << "NUMBER " << lexer.getNumber() << "\n";
          break;
        case Token::PROC:
          std::cout << "PROC\n";
          break;
        case Token::UNKNOWN:
          std::cout << "UNKNOWN\n";
          break;
        case Token::END_OF_FILE:
          std::cout << "EOF\n";
          std::exit(0);
      }
    }
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    std::exit(1);
  }
  return 0;
}
