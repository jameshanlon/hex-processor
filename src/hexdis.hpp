#ifndef HEX_DIS_HPP
#define HEX_DIS_HPP

#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "hex.hpp"

namespace hexdis {

struct DebugInfo {
  std::map<unsigned, std::string> labelMap; // byte offset -> label name
};

/// Read a hex binary file into program bytes and optional debug info.
inline void readBinary(std::istream &file, std::streampos fileSize,
                       std::vector<uint8_t> &program, DebugInfo &debugInfo) {
  // Read program size (first 4 bytes, in words).
  uint32_t programSizeWords;
  file.read(reinterpret_cast<char *>(&programSizeWords), 4);
  uint32_t programSizeBytes = programSizeWords << 2;

  // Read program bytes.
  program.resize(programSizeBytes);
  file.read(reinterpret_cast<char *>(program.data()), programSizeBytes);

  // Read debug info (if present).
  unsigned remainingFileSize = static_cast<unsigned>(fileSize) - 4;
  remainingFileSize = (remainingFileSize + 3U) & ~3U;
  if (remainingFileSize > programSizeBytes) {
    // Strings.
    uint32_t numStrings;
    file.read(reinterpret_cast<char *>(&numStrings), sizeof(uint32_t));
    std::vector<std::string> strings;
    for (size_t i = 0; i < numStrings; i++) {
      char c = file.get();
      std::string s;
      while (c != '\0') {
        s += c;
        c = file.get();
      }
      strings.push_back(s);
    }
    // Symbols.
    uint32_t numSymbols;
    file.read(reinterpret_cast<char *>(&numSymbols), sizeof(uint32_t));
    for (size_t i = 0; i < numSymbols; i++) {
      uint32_t strIndex;
      uint32_t byteOffset;
      file.read(reinterpret_cast<char *>(&strIndex), sizeof(uint32_t));
      file.read(reinterpret_cast<char *>(&byteOffset), sizeof(uint32_t));
      if (strIndex < strings.size()) {
        debugInfo.labelMap[byteOffset] = strings[strIndex];
      }
    }
  }
}

/// Read a hex binary from a file path.
inline void readBinaryFile(const char *filename, std::vector<uint8_t> &program,
                           DebugInfo &debugInfo) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error(std::string("could not open file: ") + filename);
  }
  file.seekg(0, std::ios::end);
  auto fileSize = file.tellg();
  file.seekg(0, std::ios::beg);
  readBinary(file, fileSize, program, debugInfo);
}

/// Disassemble program bytes to an output stream.
inline void disassemble(const std::vector<uint8_t> &program, std::ostream &out,
                        const DebugInfo &debugInfo, bool showLabels = true) {
  uint32_t oreg = 0;
  for (uint32_t pc = 0; pc < program.size(); pc++) {
    uint8_t byte = program[pc];
    auto instrEnum = static_cast<hex::Instr>((byte >> 4) & 0xF);
    unsigned operand = byte & 0xF;
    oreg = oreg | operand;

    // Print label if present.
    if (showLabels && debugInfo.labelMap.count(pc)) {
      out << fmt::format("\n{}:\n", debugInfo.labelMap.at(pc));
    }

    // Print instruction.
    if (instrEnum == hex::Instr::PFIX) {
      out << fmt::format("  {:#06x}  {:02x}  {:<4} {}\n", pc, byte,
                         hex::instrEnumToStr(instrEnum), operand);
      oreg = oreg << 4;
    } else if (instrEnum == hex::Instr::NFIX) {
      out << fmt::format("  {:#06x}  {:02x}  {:<4} {}\n", pc, byte,
                         hex::instrEnumToStr(instrEnum), operand);
      oreg = 0xFFFFFF00 | (oreg << 4);
    } else if (instrEnum == hex::Instr::OPR) {
      auto oprInstr = static_cast<hex::OprInstr>(oreg);
      out << fmt::format("  {:#06x}  {:02x}  {}\n", pc, byte,
                         hex::oprInstrEnumToStr(oprInstr));
      oreg = 0;
    } else {
      out << fmt::format("  {:#06x}  {:02x}  {:<4} {}\n", pc, byte,
                         hex::instrEnumToStr(instrEnum), oreg);
      oreg = 0;
    }
  }
}

} // End namespace hexdis.

#endif // HEX_DIS_HPP
