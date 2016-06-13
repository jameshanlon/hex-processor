#include <cassert>
#include <cstdlib>
#include <map>
#include <vector>
#include <iostream>

// The lexical analyser
class Lex {
  const int BUFFER_SIZE = 64;
  Table table;
  std::ifstream &file;
  int lineNum;
  int linePos;
  int value;
  std::string s;
  char ch;
  int count;
  char buffer[BUFFER_SIZE];
  bool nlPending;

  void error(const char *msg) {
    printf("Error near line %d: %s\n", lineNum, msg);
    printChBuf();
    ERR.record();
    std::exit(1);
    // Skip up to a safer point
    //Lex::Token t = readToken();
    //LEX.printToken(t);
    //while (t != Token::EOF &&
    //       t != Token::SEMI &&
    //       t != Token::AND &&
    //       t != Token::RCURLY &&
    //       t != Token::LCURLY)
    //  t = readToken();
  }

  void declare(const char *keyword, Lex::Token t) {
    table.insert(std::string(keyword), t);
  }

  void readChar() {
    ch = std::fgetc(file);
    chBuf[++count % BUFFER_SIZE] = ch;
  }

  void printChBuf() {
    std::cout << "\n...";
    for(int i = count + 1; i < count + BUFFER_SIZE; ++i) {
      if (char c = chBuf[i % BUFFER_SIZE]) {
        std::cout << c;
      }
    }
    std::cout << '\n';
  }

  void skipLine() {
    do readChar(); while (ch != EOF && ch != '\n');
    if (ch=='\n') {
      ++lineNum;
    }
  }

  void readDecInt() {
    s.clear();
    do {
      s += ch;
      readChar();
    } while ('0' <= ch && ch <= '9');
    value = std::stoi(s, nullptr, 0);
  }

  void readHexInt() {
    s.clear();
    do {
      s += ch;
      readChar();
    } while(('0' <= ch && ch <= '9')
         || ('a' <= ch && ch <= 'z')
         || ('A' <= ch && ch <= 'Z'));
    value = std::stoi(s, nullptr, 16);
  }

  void readBinInt() {
    s.clear();
    do {
      s += ch;
      readChar();
    } while('0' <= ch && ch <= '1');
    value = std::stoi(s, nullptr, 2);
  }

  void readName() {
    s.clear();
    s = ch;
    readChar();
    while(('a' <= ch && ch <= 'z') ||
          ('A' <= ch && ch <= 'Z') ||
          ('0' <= ch && ch <= '9') ||
          ch == '_') {
      s += ch;
      readChar();
    }
  }

  char readStrCh() {
    char res = ch;
    if(ch == '\n') {
      ++lineNum;
      error("expected ''' after character constant");
    }
    if(ch == '\\') {
      readChar();
      switch(ch) {
      default: error("bad string or character constant");
      case '\\': case '\'': case '"':
        res = ch;
        break;
      case 't':
      case 'T':
        res = '\t';
        break;
      case 'r':
      case 'R':
        res = '\r';
        break;
      case 'n':
      case 'N':
        res = '\n';
        break;
      }
    }
    readChar();
    return res;
  }

  void declareKeywords() {
    declare("do",       Lex::Token::DO);
    declare("else",     Lex::Token::ELSE);
    declare("false",    Lex::Token::FALSE);
    declare("for",      Lex::Token::FOR);
    declare("function", Lex::Token::FUNC);
    declare("if",       Lex::Token::IF);
    declare("is",       Lex::Token::IS);
    declare("process",  Lex::Token::PROC);
    declare("result",   Lex::Token::RESULT);
    declare("skip",     Lex::Token::SKIP);
    declare("step",     Lex::Token::STEP);
    declare("stop",     Lex::Token::STOP);
    declare("then",     Lex::Token::THEN);
    declare("to",       Lex::Token::TO);
    declare("true",     Lex::Token::TRUE);
    declare("val",      Lex::Token::VAL);
    declare("valof",    Lex::Token::VALOF);
    declare("var",      Lex::Token::VAR);
    declare("while",    Lex::Token::WHILE);
  }

public:
  enum class Token {
    EOF=1,
    ERROR,
    // Literals
    DECINT,  HEXINT, BININT, NAME,
    // Symbols
    LCURLY,  RCURLY, LSQ,    RSQ,
    LPAREN,  RPAREN, COMMA,  DOT,
    SEMI,    EQ,     ADD,    SUB,
    MUL,     DIV,    XOR,    REM,
    NEQ,     NOT,    LEQ,    LSH,
    LT,      GEQ,    RSH,    GT,
    ASS,     LAND,   AND,    LOR,
    OR,      STR,    CHAR,
    // Keywords
    DO,      ELSE,   FALSE,  FOR,
    FUNC,    IF,     RESULT, SKIP,
    STEP,    STOP,   THEN,   TO,
    TRUE,    VAL,    VALOF,  VAR,
    WHILE,   CASE,
  } token;

  Lex(Table &table, std::ifstream &file) :
      table(table),
      file(file),
      lineNum(1),
      count(0) {
    declareKeywords();
    readChar();
  }

  Token readToken() {
    Lex::Token token;
    switch(ch) {

    // Newlines (skip)
    case '\n':
      lineNum++;
      readChar();
      return readToken();

    // Whitespace (skip)
    case '\r': case '\t': case ' ':
      do readChar(); while (ch == '\r' || ch == '\t' || ch == ' ');
      return readToken();

    // Comment: #.* (skip)
    case '#':
      do readChar(); while (ch != EOF && ch != '\n');
      if(ch == '\n') {
        lineNum++;
      }
      readChar();
      return readToken();

    // Number literal: [0-9]+
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
      readDecInt();
      return Token::DECINT;

    // Number literals: hex, octal and binary
    case '0':
      readChar();
      if (ch == 'x') {
        readHexInt();
        token = Token::HEXINT;
        break;
      }
      if (ch == 'b') {
        readBinInt();
        token = Token::BININT;
        break;
      }
      error("expected 0x or 0b literal");
      return Token::ERROR;

    // Name: [a-zA-Z][a-zA-Z0-9]*
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z':
      readName();
      return (Lex::Token) table.lookup(s);

    // Symbols
    case '{': token = Token::LCURLY;  break;
    case '}': token = Token::RCURLY;  break;
    case '[': token = Token::LSQ;     break;
    case ']': token = Token::RSQ;     break;
    case '(': token = Token::LPAREN;  break;
    case ')': token = Token::RPAREN;  break;
    case ',': token = Token::COMMA;   break;
    case '.': token = Token::DOT;     break;
    case ';': token = Token::SEMI;    break;
    case '=': token = Token::EQ;      break;
    case '+': token = Token::ADD;     break;
    case '-': token = Token::SUB;     break;
    case '*': token = Token::MUL;     break;
    case '/': token = Token::DIV;     break;
    case '^': token = Token::XOR;     break;
    case '%': token = Token::REM;     break;

    case '~':
      readChar();
      if(ch == '=') { token = Token::NEQ; break; }
      return Token::NOT;

    case '<':
      readChar();
      if (ch == '=') { token = Token::LEQ; break; }
      if (ch == '<') { token = Token::LSH; break; }
      return Token::LT;

    case '>':
      readChar();
      if (ch == '=') { token = Token::GEQ; break; }
      if (ch == '>') { token = Token::RSH; break; }
      return Token::GT;

    case ':':
      readChar();
      if (ch == '=') { token = Token::ASS; break; }
      return Token::COLON;

    case '&':
      readChar();
      if(ch == '&') { token = Token::LAND; break; }
      return Token::AND;

    case '|':
      if(ch == '|') { token = Token::LOR; break; }
      return Token::OR;

    case '"':
      s.clear();
      while (ch != '"' && ch != EOF) {
        s += readStrCh();
      }
      token = Token::STR;
      break;

    case '\'':
      readChar();
      value = static_cast<int>(ch);
      ch = readStrCh();
      if(ch == '\'') { token = Token::CHAR; break; }
      error("expected ''' after character constant");
      return Token::ERROR;

    // EOF or invalid tokens
    default:
      if(ch != EOF) {
        error("illegal character");
        // Skip the rest of the line
        do readChar(); while (ch != EOF && ch != '\n');
        return Token::ERROR;
      }
      return Token::EOF;
    }

    readChar();
    return token;
  }

  void printToken(Token token) {
    std::cout << "token " << t << " " << tokStr(token);
    switch (t) {
    case Lex::Token::NAME:
      std::cout << s;
      break;
    case Lex::Token::STR:
      std::cout << s;
      break;
    case Lex::Token::DECINT:
      std::cout << value;
      break;
    case Lex::Token::HEXINT:
      std::cout << std::hex << value << std::dec;
      break;
    case Lex::Token::BININT: {
        int val = value;
        char s[BUF_SIZE + 1];
        char *p = s + BUF_SIZE;
        do { *--p = '0' + (val & 1); } while (val >>= 1);
        std::cout << s;
        break;
      }
    }
    std::cout << std::endl;
  }

  const char *Lex::tokStr(Lex::Token t) {
    switch(t) {
      default:            return "unknown";
      case Token::ERROR:  return "error";
      case Token::EOF:    return "EOF";
      // Literals
      case Token::DECINT: return "decimal";
      case Token::HEXINT: return "hexdecimal";
      case Token::BININT: return "binary";
      case Token::NAME:   return "name";
      // Symbols
      case Token::LCURLY: return "{";
      case Token::RCURLY: return "}";
      case Token::LSQ:    return "[";
      case Token::RSQ:    return "]";
      case Token::LPAREN: return "(";
      case Token::RPAREN: return ")";
      case Token::COMMA:  return ",";
      case Token::DOT:    return ".";
      case Token::SEMI:   return ";";
      case Token::EQ:     return "=";
      case Token::ADD:    return "+";
      case Token::SUB:    return "-";
      case Token::MUL:    return "*";
      case Token::DIV:    return "/";
      case Token::XOR:    return "^";
      case Token::REM:    return "%";
      case Token::NEQ:    return "~=";
      case Token::NOT:    return "~";
      case Token::LEQ:    return "<=";
      case Token::LSH:    return "<<";
      case Token::LT:     return "<";
      case Token::GEQ:    return ">=";
      case Token::RSH:    return ">>";
      case Token::GT:     return ">";
      case Token::ASS:    return ":=";
      case Token::COLON:  return ":";
      case Token::LAND:   return "&&";
      case Token::AND:    return "&";
      case Token::LOR:    return "||";
      case Token::OR:     return "|";
      case Token::STR:    return "string";
      case Token::CHAR:   return "char";
      // Keywords
      case Token::DO:     return "do";
      case Token::ELSE:   return "else";
      case Token::FALSE:  return "false";
      case Token::FOR:    return "for";
      case Token::FUNC:   return "function";
      case Token::IF:     return "if";
      case Token::IS:     return "is";
      case Token::PROC:   return "process";
      case Token::RESULT: return "result";
      case Token::SKIP:   return "skip";
      case Token::STEP:   return "step";
      case Token::STOP:   return "stop";
      case Token::THEN:   return "then";
      case Token::TO:     return "to";
      case Token::TRUE:   return "true";
      case Token::VAL:    return "val";
      case Token::VALOF:  return "valof";
      case Token::VAR:    return "var";
      case Token::WHILE:  return "while";
    }
  }
};

// Symbol table.
class Table {
  std::map<std::string, Lex::Token> table;
public:
  void insert(const std::string &name, const Lex::Token token) {
    table.insert(std::make_pair(name, token));
  }
  Lex::Token lookup(const std::string &name) {
    std::map<std::string, Lex::Token>::const_iterator it = table.find(name);
    if(it != table.end())
      return it->second;
    table.insert(std::make_pair(name, Lex::Token::NAME));
    return Lex::Token::NAME;
  }
};

static void help(int *argv[]) {
  std::cout << "X language compiler\n\n";
  std::cout << "Usage: " << argv[0] << " <source>\n\n";
  std::cout << "Options:\n";
  std::cout << "  -h display this message\n";
  std::cout << "  -l print the lexical tokens\n";
  std::cout << "  -t print the syntax tree\n";
  std::exit(1);
}

int main(int argc, const char *argv[]) {
  const char *filename = nullptr;
  bool printTokens = true;
  bool printTree = true;
  for (unsigned i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "-h") == 0) {
      help(argv);
    } else if(!strcmp(argv[i], "-l")) {
      printTokens = true;
    } else if(!strcmp(argv[i], "-t")) {
      printTree = true;
    } else {
      if (!filename) {
        filename = argv[i];
      } else {
        error("cannot specify more than one source");
      }
    }
  }
  if (!filename) {
    help(argv);
  }
  std::ifstream file(filename);
  Table table;
  Lex(table, file);
  return 0;
}

