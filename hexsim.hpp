#ifndef HEX_SIM_HPP
#define HEX_SIM_HPP

#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <map>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>

#include "hex.hpp"
#include "hexsimio.hpp"

namespace hexsim {

class Processor {

  // Constants.
  static const size_t MEMORY_SIZE_WORDS = 200000;

  // State.
  uint32_t pc;
  uint32_t areg;
  uint32_t breg;
  uint32_t oreg;
  uint32_t instr;

  // Memory.
  std::array<uint32_t, MEMORY_SIZE_WORDS> memory;

  // IO.
  hex::HexSimIO io;

  // Logging.
  std::ostream &out;

  // Control.
  bool running;
  bool tracing;
  int exitCode;

  // State for tracing.
  uint32_t lastPC;
  size_t cycles;
  size_t maxCycles;
  hex::Instr instrEnum;
  std::vector<std::pair<std::string, unsigned>> debugInfo;
  std::map<std::string, unsigned> debugInfoMap;

  /// Lookup a symbol name given the current PC.
  const char *lookupSymbol() {
    if (lastPC < debugInfo[0].second) {
      return nullptr;
    }
    for (size_t i=0; i<debugInfo.size(); i++) {
      if (i == debugInfo.size() - 1 && lastPC >= debugInfo[i].second) {
        return debugInfo[i].first.c_str();
      }
      if (lastPC >= debugInfo[i].second && lastPC < debugInfo[i+1].second) {
        return debugInfo[i].first.c_str();
      }
    }
    return nullptr;
  }

public:

  Processor(std::istream &in, std::ostream &out, size_t maxCycles=0) :
    pc(0), areg(0), breg(0), oreg(0), io(in, out), out(out),
    running(true), tracing(false), lastPC(0), cycles(0),
    maxCycles(maxCycles) {}

  void setTracing(bool value) { tracing = value; }

  void load(const char *filename, bool dumpContents=false) {
    // Load the binary file.
    std::streampos fileSize;
    std::ifstream file(filename, std::ios::binary);

    // Get length of file.
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Check the file length matches.
    unsigned remainingFileSize = static_cast<unsigned>(fileSize) - 4;
    remainingFileSize = (remainingFileSize + 3U) & ~3U; // Round up to multiple of 4.
    unsigned programSize;
    file.read(reinterpret_cast<char*>(&programSize), 4);
    programSize <<= 2;
    //if (programSize != remainingFileSize) {
    //  std::cerr << boost::format("Warning: mismatching program size %d != %d\n")
    //                 % programSize % remainingFileSize;
    //}

    // Read the instructions into memory.
    file.read(reinterpret_cast<char*>(memory.data()), programSize);

    // Read debug data (if present).
    if (remainingFileSize > programSize) {
      // Strings.
      uint32_t numStrings;
      file.read(reinterpret_cast<char*>(&numStrings), sizeof(uint32_t));
      //std::cout << std::to_string(numStrings) << " strings\n";
      std::vector<std::string> strings;
      for (size_t i=0; i<numStrings; i++) {
        char c = file.get();
        std::string s;
        while (c != '\0') {
          s += c;
          c = file.get();
        }
        strings.push_back(s);
      }
      // Symbols
      uint32_t numSymbols;
      file.read(reinterpret_cast<char*>(&numSymbols), sizeof(uint32_t));
      //std::cout << std::to_string(numSymbols) << " symbols\n";
      for (size_t i=0; i<numSymbols; i++) {
        uint32_t strIndex;
        uint32_t byteOffset;
        file.read(reinterpret_cast<char*>(&strIndex), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&byteOffset), sizeof(uint32_t));
        //std::cout << "symbol " << strings[strIndex] << " " << std::to_string(byteOffset) << "\n";
        debugInfo.push_back(std::make_pair(strings[strIndex], byteOffset));
        debugInfoMap[strings[strIndex]] = byteOffset;
      }
    }

    // Print the contents of the binary.
    if (dumpContents) {
      out << "Read " << std::to_string(programSize) << " bytes\n";
      for (size_t i=0; i<(programSize / 4) + 1; i++) {
        out << boost::format("%08d %08x\n") % i % memory[i];
      }
    }
  }

  void traceSyscall() {
    unsigned spWordIndex = memory[1];
    switch (static_cast<hex::Syscall>(areg)) {
      case hex::Syscall::EXIT:
        out << boost::format("exit %d\n") % memory[spWordIndex+2];
        break;
      case hex::Syscall::WRITE:
        out << boost::format("write %d to simout(%d)\n") % memory[spWordIndex+2] % memory[spWordIndex+3];
        break;
      case hex::Syscall::READ:
        out << boost::format("read to mem[%08x]\n") % (spWordIndex+1);
        break;
    }
  }

  void trace(uint32_t instr, hex::Instr instrEnum) {
    if (debugInfo.size()) {
      auto symbolName = lookupSymbol();
      std::string symbolInfo;
      if (symbolName) {
        auto symbolOffset = lastPC - debugInfoMap[symbolName];
        symbolInfo = (boost::format("%s+%d") % symbolName % symbolOffset).str();
      }
      out << boost::format("%-6d %-6d %-12s %-4s %-2d ")
               % cycles % lastPC % symbolInfo % instrEnumToStr(instrEnum) % (instr & 0xF);
    } else {
      out << boost::format("%-6d %-6d %-4s %-2d ")
               % cycles % lastPC % instrEnumToStr(instrEnum) % (instr & 0xF);
    }
    switch (instrEnum) {
      case hex::Instr::LDAM:
        out << boost::format("areg = mem[oreg (%#08x)] (%d)\n") % oreg % memory[oreg];
        break;
      case hex::Instr::LDBM:
        out << boost::format("breg = mem[oreg (%#08x)] (%d)\n") % oreg % memory[oreg];
        break;
      case hex::Instr::STAM:
        out << boost::format("mem[oreg (%#08x)] = areg %d\n") % oreg % areg;
        break;
      case hex::Instr::LDAC:
        out << boost::format("areg = oreg %d\n") % oreg;
        break;
      case hex::Instr::LDBC:
        out << boost::format("breg = oreg %d\n") % oreg;
        break;
      case hex::Instr::LDAP:
        out << boost::format("areg = pc (%d) + oreg (%d) %d\n") % pc % oreg % (pc + oreg);
        break;
      case hex::Instr::LDAI:
        out << boost::format("areg = mem[areg (%d) + oreg (%d) = %#08x] (%d)\n") % areg % oreg % (areg+oreg) % memory[areg+oreg];
        break;
      case hex::Instr::LDBI:
        out << boost::format("breg = mem[breg (%d) + oreg (%d) = %#08x] (%d)\n") % breg % oreg % (breg+oreg) % memory[breg+oreg];
        break;
      case hex::Instr::STAI:
        out << boost::format("mem[breg (%d) + oreg (%d) = %#08x] = areg (%d)\n") % breg % oreg % (breg+oreg) % areg;
        break;
      case hex::Instr::BR:
        out << boost::format("pc = pc + oreg (%d) (%#08x)\n") % oreg % (pc + oreg);
        break;
      case hex::Instr::BRZ:
        out << boost::format("pc = areg == zero ? pc + oreg (%d) (%#08x) : pc\n") % oreg % (pc + oreg);
        break;
      case hex::Instr::BRN:
        out << boost::format("pc = areg < zero ? pc + oreg (%d) (%#08x) : pc\n") % oreg % (pc + oreg);
        break;
      case hex::Instr::PFIX:
        out << boost::format("oreg = oreg (%d) << 4 (%#08x)\n") % oreg % (oreg << 4);
        break;
      case hex::Instr::NFIX:
        out << boost::format("oreg = 0xFFFFFF00 | oreg (%d) << 4 (%#08x)\n") % oreg % (0xFFFFFF00 | (oreg << 4));
        break;
      case hex::Instr::OPR:
        switch (static_cast<hex::OprInstr>(oreg)) {
          case hex::OprInstr::BRB:
            out << boost::format("pc = breg (%#08x)\n") % breg;
            break;
          case hex::OprInstr::ADD:
           out << boost::format("areg = areg (%d) + breg (%d) (%d)\n") % areg % breg % (areg + breg);
            break;
          case hex::OprInstr::SUB:
           out << boost::format("areg = areg (%d) - breg (%d) (%d)\n") % areg % breg % (areg - breg);
            break;
          case hex::OprInstr::SVC:
            traceSyscall();
            break;
        };
        break;
   }
  }

  void syscall() {
    unsigned spWordIndex = memory[1];
    switch (static_cast<hex::Syscall>(areg)) {
      case hex::Syscall::EXIT:
        exitCode = memory[spWordIndex+2];
        running = false;
        break;
      case hex::Syscall::WRITE:
        io.output(memory[spWordIndex+2], memory[spWordIndex+3]);
        break;
      case hex::Syscall::READ:
        memory[spWordIndex+1] = io.input(memory[spWordIndex+2]) & 0xFF;
        break;
      default:
        throw std::runtime_error("invalid syscall: " + std::to_string(areg));
    }
  }

  int run() {
    while (running &&
           (maxCycles > 0 ? cycles <= maxCycles : true)) {
      instr = (memory[pc >> 2] >> ((pc & 0x3) << 3)) & 0xFF;
      lastPC = pc;
      pc = pc + 1;
      oreg = oreg | (instr & 0xF);
      instrEnum = static_cast<hex::Instr>((instr >> 4) & 0xF);
      if (tracing) {
        trace(instr, instrEnum);
      }
      switch (instrEnum) {
        case hex::Instr::LDAM:
          areg = memory[oreg];
          oreg = 0;
          break;
        case hex::Instr::LDBM:
          breg = memory[oreg];
          oreg = 0;
          break;
        case hex::Instr::STAM:
          memory[oreg] = areg;
          oreg = 0;
          break;
        case hex::Instr::LDAC:
          areg = oreg;
          oreg = 0;
          break;
        case hex::Instr::LDBC:
          breg = oreg;
          oreg = 0;
          break;
        case hex::Instr::LDAP:
          areg = pc + oreg;
          oreg = 0;
          break;
        case hex::Instr::LDAI:
          areg = memory[areg + oreg];
          oreg = 0;
          break;
        case hex::Instr::LDBI:
          breg = memory[breg + oreg];
          oreg = 0;
          break;
        case hex::Instr::STAI:
          memory[breg + oreg] = areg;
          oreg = 0;
          break;
        case hex::Instr::BR:
          pc = pc + oreg;
          oreg = 0;
          break;
        case hex::Instr::BRZ:
          if (areg == 0) {
            pc = pc + oreg;
          }
          oreg = 0;
          break;
        case hex::Instr::BRN:
          if ((int)areg < 0) {
            pc = pc + oreg;
          }
          oreg = 0;
          break;
        case hex::Instr::PFIX:
          oreg = oreg << 4;
          break;
        case hex::Instr::NFIX:
          oreg = 0xFFFFFF00 | (oreg << 4);
          break;
        case hex::Instr::OPR:
          switch (static_cast<hex::OprInstr>(oreg)) {
            case hex::OprInstr::BRB:
              pc = breg;
              oreg = 0;
              break;
            case hex::OprInstr::ADD:
              areg = areg + breg;
              oreg = 0;
              break;
            case hex::OprInstr::SUB:
              areg = areg - breg;
              oreg = 0;
              break;
            case hex::OprInstr::SVC:
              syscall();
              break;
            default:
              throw std::runtime_error("invalid OPR: " + std::to_string(oreg));
          };
          oreg = 0;
          break;
        default:
          throw std::runtime_error("invalid instruction");
      }
      cycles++;
    }
    return exitCode;
  }
};

} // End namespace hexsim

#endif // HEX_SIM_HPP
