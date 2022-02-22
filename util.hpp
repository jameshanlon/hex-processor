#ifndef UTIL_HPP
#define UTIL_HPP

#include <fstream>
#include <string>
#include <vector>

namespace hex {

std::array<std::fstream, 8> fileIO;
std::array<bool, 8> connected = {false, false, false, false, false, false, false, false};

/// Output a character to stdout or a file.
void output(char value, int stream) {
  if (stream < 256) {
    std::cout << value;
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
    return std::getchar();
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

}; // End namespace hex.

#endif // UTIL_HPP