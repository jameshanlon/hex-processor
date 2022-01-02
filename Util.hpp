#ifndef UTIL_HPP
#define UTIL_HPP

#include <fstream>
#include <string>
#include <vector>

/// Output a character to stdout or a file.
void output(std::vector<std::fstream> &fileIO, char value, int stream) {
  if (stream < 256) {
    std::cout << value;
  } else {
    size_t index = (stream >> 8) & 7;
    if (!fileIO[index].is_open()) {
      fileIO[index].open(std::string("simin")+std::to_string(index),
                         std::fstream::in);
    }
    fileIO[index].put(value);
  }
}

/// Input a character from stdin or a file.
char input(std::vector<std::fstream> &fileIO, int stream) {
  if (stream < 256) {
    return std::getchar();
  } else {
    size_t index = (stream >> 8) & 7;
    if (!fileIO[index].is_open()) {
      fileIO[index].open(std::string("simin")+std::to_string(index),
                         std::fstream::out);
    }
    return fileIO[index].get();
  }
}

#endif // UTIL_HPP

