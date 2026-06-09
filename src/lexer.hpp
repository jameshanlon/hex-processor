#ifndef HEX_LEXER_HPP
#define HEX_LEXER_HPP

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <istream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "util.hpp"

namespace hexlex {

/// Shared character-stream layer for the assembler and compiler lexers. It owns
/// the input stream and the line/column tracking, and provides the common
/// scanning helpers; derived classes supply language-specific token recognition
/// by overriding readToken(). TokenT is the language's token enumeration.
template <typename TokenT> class LexerBase {
protected:
  std::unique_ptr<std::istream> file;
  char lastChar = 0;
  std::string identifier;
  unsigned value = 0;
  TokenT lastToken{};
  size_t currentLineNumber = 0;
  size_t currentCharNumber = 0;
  std::string currentLine;

  /// Read the next character, tracking the current line and column.
  int readChar() {
    file->get(lastChar);
    currentLine += lastChar;
    if (file->eof()) {
      lastChar = EOF;
    }
    currentCharNumber++;
    return lastChar;
  }

  /// Reset the per-line tracking state on crossing a newline.
  void newLine() {
    currentLineNumber++;
    currentCharNumber = 0;
    currentLine.clear();
  }

  /// Skip whitespace until lastChar is a non-space character.
  void skipWhitespace() {
    while (std::isspace(lastChar)) {
      if (lastChar == '\n') {
        newLine();
      }
      readChar();
    }
  }

  /// Scan an identifier into `identifier` (the first alpha is at lastChar).
  void readIdentifier() {
    identifier = std::string(1, lastChar);
    while (std::isalnum(readChar()) || lastChar == '_') {
      identifier += lastChar;
    }
  }

  /// Scan a decimal integer into `value` (the first digit is at lastChar).
  void readDecInt() {
    std::string number(1, lastChar);
    while (std::isdigit(readChar())) {
      number += lastChar;
    }
    value = std::strtoul(number.c_str(), nullptr, 10);
  }

  /// Close the underlying stream if it is a file.
  void closeIfFile() {
    if (auto *ifstream = dynamic_cast<std::ifstream *>(file.get())) {
      ifstream->close();
    }
  }

  /// Recognise and return the next token. Implemented by each language's lexer.
  virtual TokenT readToken() = 0;

public:
  virtual ~LexerBase() = default;

  TokenT getNextToken() { return lastToken = readToken(); }

  /// Open a file for lexing.
  void openFile(const char *filename) {
    auto ifstream = std::make_unique<std::ifstream>();
    ifstream->open(filename, std::ifstream::in);
    if (!ifstream->is_open()) {
      throw std::runtime_error("could not open file");
    }
    file.reset(ifstream.release());
    readChar();
  }
  void openFile(const std::string &filename) { openFile(filename.c_str()); }

  /// Lex from an in-memory string.
  void loadBuffer(const std::string &buffer) {
    file = std::make_unique<std::istringstream>(buffer);
    readChar();
  }

  const std::string &getIdentifier() const { return identifier; }
  unsigned getNumber() const { return value; }
  TokenT getLastToken() const { return lastToken; }
  size_t getLineNumber() const { return currentLineNumber; }
  size_t getCharNumber() const { return currentCharNumber; }
  bool hasLine() const { return !currentLine.empty(); }
  const std::string &getLine() const { return currentLine; }
  hexutil::Location getLocation() const {
    return hexutil::Location(currentLineNumber, currentCharNumber);
  }
};

} // namespace hexlex

#endif // HEX_LEXER_HPP
