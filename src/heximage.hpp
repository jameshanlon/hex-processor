#ifndef HEX_IMAGE_HPP
#define HEX_IMAGE_HPP

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

//===---------------------------------------------------------------------===//
// Shared serialisation for the binary image format, so the writer (hexasm) and
// the readers (hexsim, hexdis) cannot drift.
//
// An image is: uint32 programSizeWords, programSizeWords*4 bytes of code, then
// an optional debug-info block:
//   uint32 numStrings, numStrings null-terminated strings
//   uint32 numSymbols, numSymbols * (uint32 stringIndex, uint32 byteOffset)
//===---------------------------------------------------------------------===//

namespace heximage {

/// Read a little-endian uint32 from a stream.
inline uint32_t readU32(std::istream &in) {
  uint32_t value = 0;
  in.read(reinterpret_cast<char *>(&value), sizeof(uint32_t));
  return value;
}

/// Write a little-endian uint32 to a stream.
inline void writeU32(std::ostream &out, uint32_t value) {
  out.write(reinterpret_cast<const char *>(&value), sizeof(uint32_t));
}

/// A debug symbol: a name and the byte offset it labels.
struct Symbol {
  std::string name;
  uint32_t offset;
};

/// Read the debug-info block (string table + symbol table) that follows the
/// program in an image, resolving each symbol's name from the string table.
inline std::vector<Symbol> readSymbols(std::istream &in) {
  uint32_t numStrings = readU32(in);
  std::vector<std::string> strings;
  strings.reserve(numStrings);
  for (uint32_t i = 0; i < numStrings; i++) {
    std::string s;
    char c = in.get();
    while (c != '\0') {
      s += c;
      c = in.get();
    }
    strings.push_back(std::move(s));
  }
  uint32_t numSymbols = readU32(in);
  std::vector<Symbol> symbols;
  symbols.reserve(numSymbols);
  for (uint32_t i = 0; i < numSymbols; i++) {
    uint32_t strIndex = readU32(in);
    uint32_t offset = readU32(in);
    if (strIndex < strings.size()) {
      symbols.push_back({strings[strIndex], offset});
    }
  }
  return symbols;
}

/// Write the debug-info block (string table + symbol table), emitting one
/// string per symbol (no string pooling).
inline void writeSymbols(std::ostream &out,
                         const std::vector<Symbol> &symbols) {
  auto count = static_cast<uint32_t>(symbols.size());
  writeU32(out, count);
  for (const auto &symbol : symbols) {
    out.write(symbol.name.c_str(), symbol.name.length() + 1);
  }
  writeU32(out, count);
  for (uint32_t i = 0; i < count; i++) {
    writeU32(out, i);
    writeU32(out, symbols[i].offset);
  }
}

} // namespace heximage

#endif // HEX_IMAGE_HPP
