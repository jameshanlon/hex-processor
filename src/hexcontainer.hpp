#ifndef HEX_CONTAINER_HPP
#define HEX_CONTAINER_HPP

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "heximage.hpp"

//===---------------------------------------------------------------------===//
// Reader for the HEXN network container format, shared by the C++ simulator
// (hexsim) and the Verilator testbench (hextb) so they cannot drift.
//
// Layout (little-endian):
//   uint32 magic = 0x4E584548 ("HEXN")
//   uint32 numProcessors
//   uint32 numEdges
//   edges[numEdges]: uint32 procA, slotA, procB, slotB
//   images[numProcessors]: uint32 imageSizeBytes, then imageSizeBytes of a
//     standard single-image binary (size-word + code + debug info).
//
// A file without the magic is treated as a single plain image.
//===---------------------------------------------------------------------===//

namespace hexcontainer {

constexpr uint32_t MAGIC = 0x4E584548; // "HEXN"

struct Edge {
  uint32_t procA, slotA, procB, slotB;
};

struct Container {
  bool isNetwork = false;                // false => single plain image
  std::vector<Edge> edges;               // channel wiring (network only)
  std::vector<std::vector<char>> images; // per-processor image bytes
};

using heximage::readU32;

/// Read a container file. If it lacks the HEXN magic, returns a single-image
/// container (isNetwork = false, one image holding the whole file).
inline Container read(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("could not open file: " + filename);
  }
  file.seekg(0, std::ios::end);
  auto fileSize = static_cast<size_t>(file.tellg());
  file.seekg(0, std::ios::beg);

  Container container;
  uint32_t magic = readU32(file);
  if (magic != MAGIC) {
    // Single plain image: the whole file is one image.
    file.seekg(0, std::ios::beg);
    std::vector<char> image(fileSize);
    file.read(image.data(), fileSize);
    container.images.push_back(std::move(image));
    return container;
  }

  container.isNetwork = true;
  uint32_t numProcessors = readU32(file);
  uint32_t numEdges = readU32(file);
  container.edges.resize(numEdges);
  for (auto &e : container.edges) {
    e.procA = readU32(file);
    e.slotA = readU32(file);
    e.procB = readU32(file);
    e.slotB = readU32(file);
  }
  for (uint32_t i = 0; i < numProcessors; i++) {
    uint32_t imageSize = readU32(file);
    std::vector<char> image(imageSize);
    file.read(image.data(), imageSize);
    container.images.push_back(std::move(image));
  }
  return container;
}

} // namespace hexcontainer

#endif // HEX_CONTAINER_HPP
