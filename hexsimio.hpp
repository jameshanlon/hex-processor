#ifndef HEX_SIM_IO_HPP
#define HEX_SIM_IO_HPP

#include <fstream>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace hex {

class HexSimIO {

  std::istream &in;
  std::ostream &out;
  std::array<std::fstream, 8> fileIO;
  std::array<bool, 8> connected;

public:

  HexSimIO(std::istream &in, std::ostream &out) :
      in(in), out(out), connected({false, false, false, false, false, false, false, false}) {}

  /// Output a character to ostream or a file.
  void output(char value, int stream) {
    if (stream < 256) {
      out << value;
    } else {
      size_t index = (stream >> 8) & 7;
      if (!connected[index]) {
        fileIO[index].open(std::string("simout")+std::to_string(index),
                           std::fstream::out);
        connected[index] = true;
      }
      fileIO[index].put(value);
    }
  }

  /// Input a character from stdin or a file.
  char input(int stream) {
    if (stream < 256) {
      // FIXME
      char c;
      in.get(c);
      return c;
      //return std::getchar();
    } else {
      size_t index = (stream >> 8) & 7;
      if (!connected[index]) {
        fileIO[index].open(std::string("simin")+std::to_string(index),
                           std::fstream::in);
        connected[index] = true;
      }
      return fileIO[index].get();
    }
  }

};

} // End namespace hex.

#endif // HEX_SIM_IO_HPP
