#ifndef HEX_SIM_IO_HPP
#define HEX_SIM_IO_HPP

#include <array>
#include <fstream>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace hex {

class HexSimIO {

  // Number of file-backed I/O streams.
  static constexpr size_t NUM_IO_STREAMS = 8;
  // Stream ids below this map to stdin/stdout; ids at or above it select a
  // file, with the file index held in bits [10:8].
  static constexpr int FILE_STREAM_BASE = 256;
  static constexpr int FILE_INDEX_SHIFT = 8;

  std::istream &in;
  std::ostream &out;
  std::array<std::fstream, NUM_IO_STREAMS> fileIO;
  std::array<bool, NUM_IO_STREAMS> connected;

  /// Extract the file index encoded in a (file-backed) stream id.
  static size_t fileIndex(int stream) {
    return (stream >> FILE_INDEX_SHIFT) & (NUM_IO_STREAMS - 1);
  }

public:
  HexSimIO(std::istream &in, std::ostream &out)
      : in(in), out(out),
        connected({false, false, false, false, false, false, false, false}) {}

  /// Output a character to ostream or a file.
  void output(char value, int stream) {
    if (stream < FILE_STREAM_BASE) {
      out << value;
    } else {
      size_t index = fileIndex(stream);
      if (!connected[index]) {
        fileIO[index].open(std::string("simout") + std::to_string(index),
                           std::fstream::out);
        connected[index] = true;
      }
      fileIO[index].put(value);
    }
  }

  /// Input a character from stdin or a file.
  char input(int stream) {
    if (stream < FILE_STREAM_BASE) {
      return in.get();
    } else {
      size_t index = fileIndex(stream);
      if (!connected[index]) {
        fileIO[index].open(std::string("simin") + std::to_string(index),
                           std::fstream::in);
        connected[index] = true;
      }
      return fileIO[index].get();
    }
  }
};

} // End namespace hex.

#endif // HEX_SIM_IO_HPP
