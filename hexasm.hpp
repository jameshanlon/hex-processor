#ifndef HEX_ASM_HPP
#define HEX_ASM_HPP

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <boost/format.hpp>

#include "hex.hpp"

// An assembler for the Hex instruction set, based on xhexb.x and with
// inspiration from the LLVM Kaleidoscope tutorial:
//   https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html

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

namespace hexasm {

//===---------------------------------------------------------------------===//
// Token enumeration and helper functions
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
  PADDING,
  NONE,
  END_OF_FILE
};

static const char *tokenEnumStr(Token token) {
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
  case Token::PADDING:     return "PADDING";
  case Token::NONE:        return "NONE";
  case Token::END_OF_FILE: return "END_OF_FILE";
  default:
    throw std::runtime_error(std::string("unexpected token: ")+std::to_string(static_cast<int>(token)));
  }
};

static hex::Instr tokenToInstr(Token token) {
  switch (token) {
  case Token::LDAM: return hex::Instr::LDAM;
  case Token::LDBM: return hex::Instr::LDBM;
  case Token::STAM: return hex::Instr::STAM;
  case Token::LDAC: return hex::Instr::LDAC;
  case Token::LDBC: return hex::Instr::LDBC;
  case Token::LDAP: return hex::Instr::LDAP;
  case Token::LDAI: return hex::Instr::LDAI;
  case Token::LDBI: return hex::Instr::LDBI;
  case Token::STAI: return hex::Instr::STAI;
  case Token::BR:   return hex::Instr::BR;
  case Token::BRZ:  return hex::Instr::BRZ;
  case Token::BRN:  return hex::Instr::BRN;
  case Token::OPR:  return hex::Instr::OPR;
  default:
    throw std::runtime_error(std::string("unexpected instrucion token: ")+tokenEnumStr(token));
  }
}

static hex::OprInstr tokenToOprInstr(Token token) {
  switch (token) {
  case Token::BRB: return hex::OprInstr::BRB;
  case Token::SVC: return hex::OprInstr::SVC;
  case Token::ADD: return hex::OprInstr::ADD;
  case Token::SUB: return hex::OprInstr::SUB;
  default:
    throw std::runtime_error(std::string("unexpected operand instrucion token: ")+tokenEnumStr(token));
  }
}

static int instrToInstrOpc(hex::Instr instr) {
  return static_cast<int>(instr);
}

static int tokenToInstrOpc(Token token) {
  return static_cast<int>(tokenToInstr(token));
}

static int tokenToOprInstrOpc(Token token) {
  return static_cast<int>(tokenToOprInstr(token));
}

//===---------------------------------------------------------------------===//
// Functions for determining instruction encoding sizes.
//===---------------------------------------------------------------------===//

/// Return the number of 4-bit immediates required to represent the value.
static size_t numNibbles(int value) {
  if (value == 0) {
    return 1;
  }
  if (value < 0 && std::abs(value) < 16) {
    // Account for NFIX required to add leading 1s.
    return 2;
  }
  if (value < 0) {
    value = std::abs(value);
  }
  size_t n = 1;
  while (value >= 16) {
    value >>= 4;
    n++;
  }
  return n;
}

/// Return the length of an instruction that has a relative label reference.
/// The length of the encoding depends on the distance to the label, which in
/// turn depends on the length of the instruction. Calculate the value by
/// increasing the length until they match.
static int instrLen(int labelOffset, int byteOffset) {
  int length = 1;
  while (length < numNibbles(labelOffset - byteOffset - length)) {
    length++;
  }
  return length;
}

//===---------------------------------------------------------------------===//
// Directive data types.
//===---------------------------------------------------------------------===//

size_t numNibbles(int value);

// Base class for all directives.
struct Directive {
  Token token;
  int byteOffset;
  Directive(Token token) : token(token), byteOffset(0) {}
  virtual ~Directive() = default;
  Token getToken() const { return token; }
  void setByteOffset(unsigned value) { byteOffset = value; }
  unsigned getByteOffset() const { return byteOffset; }
  virtual bool operandIsLabel() const = 0;
  virtual size_t getSize() const = 0;
  virtual int getValue() const = 0;
  virtual std::string toString() const = 0;
};

class Data : public Directive {
  int value;
public:
  Data(Token token, int value) : Directive(token), value(value) {}
  bool operandIsLabel() const { return false; }
  size_t getSize() const { return 4; } // Data entries are always one word.
  int getValue() const { return value; }
  std::string toString() const {
    return "DATA " + std::to_string(value);
  }
};

class Func : public Directive {
  std::string identifier;
public:
  Func(Token token, std::string identifier) : Directive(token), identifier(identifier) {}
  bool operandIsLabel() const { return false; }
  size_t getSize() const { return 0; }
  int getValue() const { return 0; }
  std::string toString() const {
    return "FUNC " + identifier;
  }
};

class Proc : public Directive {
  std::string identifier;
public:
  Proc(Token token, std::string identifier) : Directive(token), identifier(identifier) {}
  bool operandIsLabel() const { return false; }
  size_t getSize() const { return 0; }
  int getValue() const { return 0; }
  std::string toString() const {
    return "PROC " + identifier;
  }
};

class Label : public Directive {
  std::string label;
  int labelValue;
public:
  Label(Token token, std::string label) : Directive(token), label(label) {}
  bool operandIsLabel() const { return false; }
  size_t getSize() const { return 0; }
  /// Update the label value and return true if it was changed.
  bool setLabelValue(int newValue) {
    int oldValue = labelValue;
    labelValue = newValue;
    return oldValue != newValue;
  }
  int getValue() const { return labelValue; }
  std::string getLabel() const { return label; }
  std::string toString() const { return label; }
};

class InstrImm : public Directive {
  int immValue;
public:
  InstrImm(Token token, int immValue) : Directive(token), immValue(immValue) {}
  bool operandIsLabel() const { return false; }
  size_t getSize() const {
    return (immValue < 0 && numNibbles(immValue) == 1) ? 2 : numNibbles(immValue);
  }
  int getValue() const { return immValue; }
  std::string toString() const {
    return std::string(tokenEnumStr(token)) + " " + std::to_string(immValue);
  }
};

class InstrLabel : public Directive {
  std::string label;
  int labelValue;
  bool relative;
public:
  InstrLabel(Token token, std::string label, bool relative) : Directive(token), label(label), relative(relative) {}
  void setLabelValue(int newValue) { labelValue = newValue; }
  bool operandIsLabel() const { return true; }
  bool isRelative() const { return relative; }
  size_t getSize() const {
    return (labelValue < 0 && numNibbles(labelValue) == 1) ? 2 : numNibbles(labelValue);
  }
  int getValue() const { return labelValue; }
  std::string getLabel() const { return label; }
  std::string toString() const {
    return std::string(tokenEnumStr(token)) + " " + label + " (" + std::to_string(labelValue) + ")";
  }
};

class InstrOp : public Directive {
  Token opcode;
public:
  InstrOp(Token token, Token opcode) : Directive(token), opcode(opcode) {
    if (opcode != Token::BRB &&
        opcode != Token::ADD &&
        opcode != Token::SUB &&
        opcode != Token::SVC) {
      throw std::runtime_error(std::string("unexpected operand to OPR ")+tokenEnumStr(opcode));
    }
  }
  bool operandIsLabel() const { return false; }
  size_t getSize() const { return 1; }
  int getValue() const { return tokenToOprInstrOpc(opcode); }
  std::string toString() const {
    return std::string("OPR ") + tokenEnumStr(opcode);
  }
};

class Padding : public Directive {
  size_t numBytes;
public:
  Padding(size_t numBytes) : Directive(Token::PADDING), numBytes(numBytes) {}
  bool operandIsLabel() const { return false; }
  size_t getSize() const { return numBytes; }
  int getValue() const { return tokenToOprInstrOpc(Token::PADDING); }
  std::string toString() const { return std::string("PADDING ") + std::to_string(numBytes); }
};

//===---------------------------------------------------------------------===//
// Lexer
//===---------------------------------------------------------------------===//

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

  Table                         table;
  std::unique_ptr<std::istream> file;
  char                          lastChar;
  std::string                   identifier;
  unsigned                      value;
  Token                         lastToken;
  size_t                        currentLineNumber;
  std::string                   currentLine;

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
    file->get(lastChar);
    currentLine += lastChar;
    if (file->eof()) {
      lastChar = EOF;
    }
    return lastChar;
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
    if (lastChar == '#') {
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
      if (auto ifstream = dynamic_cast<std::ifstream*>(file.get())) {
        ifstream->close();
      }
      return Token::END_OF_FILE;
    }
    readChar();
    return Token::NONE;
  }

public:

  Lexer() : currentLineNumber(0) {
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
  unsigned getNumber() const { return value; }
  Token getLastToken() const { return lastToken; }
  size_t getLineNumber() const { return currentLineNumber; }
  const std::string &getLine() const { return currentLine; }
};

//===---------------------------------------------------------------------===//
// Parser
//===---------------------------------------------------------------------===//

class Parser {
  Lexer &lexer;

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
        return std::make_unique<Data>(Token::DATA, parseInteger());
      case Token::FUNC:
        return std::make_unique<Func>(Token::FUNC, parseIdentifier());
      case Token::PROC:
        return std::make_unique<Proc>(Token::PROC, parseIdentifier());
      case Token::IDENTIFIER:
        return std::make_unique<Label>(Token::IDENTIFIER, lexer.getIdentifier());
      case Token::OPR:
        return std::make_unique<InstrOp>(Token::OPR, lexer.getNextToken());
      case Token::LDAM:
      case Token::LDBM:
      case Token::STAM:
      case Token::LDAC:
      case Token::LDBC: {
        auto opcode = lexer.getLastToken();
        if (lexer.getNextToken() == Token::IDENTIFIER) {
          return std::make_unique<InstrLabel>(opcode, lexer.getIdentifier(), false);
        } else {
          return std::make_unique<InstrImm>(opcode, parseInteger());
        }
      }
      case Token::LDAP:
      case Token::LDAI:
      case Token::LDBI:
      case Token::STAI:
      case Token::BR:
      case Token::BRN:
      case Token::BRZ: {
        auto opcode = lexer.getLastToken();
        if (lexer.getNextToken() == Token::IDENTIFIER) {
          return std::make_unique<InstrLabel>(opcode, lexer.getIdentifier(), true);
        } else {
          return std::make_unique<InstrImm>(opcode, parseInteger());\
        }
      }
      default:
        throw std::runtime_error(std::string("unrecognised token ")+tokenEnumStr(lexer.getLastToken()));
    }
  }

public:
  Parser(Lexer &lexer) : lexer(lexer) {}

  std::vector<std::unique_ptr<Directive>> parseProgram() {
    std::vector<std::unique_ptr<Directive>> program;
    while (lexer.getNextToken() != Token::END_OF_FILE) {
      program.push_back(parseDirective());
    }
    return program;
  }
};

//===---------------------------------------------------------------------===//
// Code generation.
//===---------------------------------------------------------------------===//

class CodeGen {

  std::vector<std::unique_ptr<Directive>> &program;
  std::map<std::string, Label*> labelMap;
  size_t programSize;

  /// Create a map of label strings to label Directives.
  std::map<std::string, Label*> createLabelMap() {
    std::map<std::string, Label*> labelMap;
    for (auto &directive : program) {
      if (directive->getToken() == Token::IDENTIFIER) {
        auto label = dynamic_cast<Label*>(directive.get());
        labelMap[label->getLabel()] = label;
      }
    }
    return labelMap;
  }

  /// Iteratively update label values until the program size does not change.
  /// Return the final size of the program.
  void resolveLabels() {
    int lastSize = -1;
    int byteOffset = 0;
    //int count = 0;
    while (lastSize != byteOffset) {
      //std::cout << "Resolving labels iteration " << count++ << "\n";
      lastSize = byteOffset;
      byteOffset = 0;
      for (auto &directive : program) {
        if (directive->getToken() == Token::DATA) {
          // Data must be on 4-byte boundaries.
          if (byteOffset & 0x3) {
            byteOffset += 4 - (byteOffset & 0x3);
          }
        }
        // Update the label value.
        if (directive->getToken() == Token::IDENTIFIER) {
          dynamic_cast<Label*>(directive.get())->setLabelValue(byteOffset);
        }
        // Update the label operand value of an instruction, accounting for
        // relative and absolute references.
        if (directive->operandIsLabel()) {
          auto instrLabel = dynamic_cast<InstrLabel*>(directive.get());
          int labelValue = labelMap[instrLabel->getLabel()]->getValue();
          if (instrLabel->isRelative()) {
            int offset = labelValue - byteOffset;
            //std::cout << "label value " << labelValue
            //          << " byteOffset " << byteOffset
            //          << " offset " << offset
            //          << " instrlen " << instrLen(labelValue, byteOffset) << "\n";
            if (offset >= 0) {
              instrLabel->setLabelValue(offset - instrLen(labelValue, byteOffset));
            } else {
              instrLabel->setLabelValue(offset - instrLen(labelValue, byteOffset));
            }
          } else {
            assert((labelValue & 0x3) == 0 && "absolute label value is not word aligned");
            instrLabel->setLabelValue(labelValue >> 2);
          }
        }
        directive->setByteOffset(byteOffset);
        byteOffset += directive->getSize();
      }
    }
  }

public:

  /// Constructor.
  CodeGen(std::vector<std::unique_ptr<Directive>> &program) :
      program(program), programSize(0) {

    // Iteratively resolve label values.
    createLabelMap();
    resolveLabels();

    // Determine the size of the program.
    auto programSize = getProgramSize();

    // Add space for padding bytes at the end.
    auto paddingBytes = ((programSize + 3U) & ~3U) - programSize;
    program.push_back(std::make_unique<Padding>(paddingBytes));
    programSize += paddingBytes;
  }

  /// Return the size of the program in bytes (after resolveLabels()).
  size_t getProgramSize() {
    return program.back()->getByteOffset() + program.back()->getSize();
  }

  /// Emit the program to an output stream.
  void emitProgramText(std::ostream &out) {
    for (auto &directive : program) {
      out << boost::format("%#08x %-20s (%d bytes)\n")
               % directive->getByteOffset()
               % directive->toString()
               % directive->getSize();
    }
    out << boost::format("%d bytes\n") % getProgramSize();
  }

  /// Emit each directive of the program as binary.
  void emitProgramBin(std::ostream &outputFile) {
    int byteOffset = 0;
    for (auto &directive : program) {
      size_t size = directive->getSize();
      // Padding
      if (directive->getToken() == Token::PADDING) {
        for (size_t i=0; i<directive->getSize(); i++) {
          outputFile.put(0);
        }
      // Data
      } else if (directive->getToken() == Token::DATA) {
        // Add padding for 4-byte data alignment.
        if (byteOffset & 0x3) {
          int paddingBytes = 4 - (byteOffset & 0x3);
          int paddingValue = 0;
          outputFile.write(reinterpret_cast<const char*>(&paddingValue), paddingBytes);
          byteOffset += paddingBytes;
        }
        auto dataDirective = dynamic_cast<Data*>(directive.get());
        auto value = dataDirective->getValue();
        outputFile.write(reinterpret_cast<const char*>(&value), size);
        byteOffset += size;
      // Instruction
      } else if (size > 0) {
        if (size > 1) {
          // Output PFIX/NFIX to extend the immediate value.
          hex::Instr instr = (directive->getValue() < 0) ? hex::Instr::NFIX : hex::Instr::PFIX;
          char instrValue = instrToInstrOpc(instr) << 4 |
                            ((directive->getValue() >> ((size - 1) * 4)) & 0xF);
          outputFile.put(instrValue);
          byteOffset++;
        }
        if (size > 2) {
          for (size_t i=size-2; i>0; i--) {
            char instrValue = instrToInstrOpc(hex::Instr::PFIX) << 4 |
                              ((directive->getValue() >> (i * 4)) & 0xF);
            outputFile.put(instrValue);
            byteOffset++;
          }
        }
        // Output the instruction
        char instrValue = (tokenToInstrOpc(directive->getToken()) & 0xF) << 4 |
                          (directive->getValue() & 0xF);
        outputFile.put(instrValue);
        byteOffset++;
      }
    }
  }

  /// Emit the binary.
  void emitBin(std::string outputFilename) {
    std::fstream outputFile(outputFilename, std::ios::out | std::ios::binary);
    // The first four bytes are the remaining binary size.
    auto programSizeWords = programSize >> 2;
    outputFile.write(reinterpret_cast<const char*>(&programSizeWords), sizeof(unsigned));
    // Emit the program.
    emitProgramBin(outputFile);
    // Done.
    outputFile.close();
  }
};

} // End namespace hexasm.

#endif // HEX_ASM_HPP
