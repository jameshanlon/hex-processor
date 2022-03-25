#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <exception>
#include <boost/format.hpp>

namespace hexutil {

/// A class to represent a source code location.
class Location {
  size_t line, position;
  bool null;
public:
  Location() : line(0), position(0), null(true) {}
  Location(size_t line, size_t position) : line(line), position(position), null(false) {}
  std::string str() const {
    return null ? "no location" : (boost::format("line %d:%d") % line % position).str();
  }
  bool isNull() const { return null; }
};

/// General error class with location information.
class Error : public std::runtime_error {
  Location location;
public:
  Error(const std::string &what) :
      std::runtime_error(what) {}
  Error(const char *what) :
      std::runtime_error(what) {}
  Error(Location location, const std::string &what) :
      std::runtime_error(what), location(location) {}
  Error(Location location, const char *what) :
      std::runtime_error(what), location(location) {}
  bool hasLocation() const { return !location.isNull(); }
  const Location &getLocation() const { return location; }
};

} // End namespace hexutil

#endif // UTIL_HPP
