#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <fmt/format.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace hexutil {

/// A class to represent a source code location. A default-constructed Location
/// represents "no location".
class Location {
  // (line, position) when present; empty represents "no location".
  std::optional<std::pair<size_t, size_t>> lineAndPosition;

public:
  Location() = default;
  Location(size_t line, size_t position)
      : lineAndPosition(std::in_place, line, position) {}
  std::string str() const {
    return lineAndPosition ? fmt::format("line {}:{}", lineAndPosition->first,
                                         lineAndPosition->second)
                           : "no location";
  }
  bool isNull() const { return !lineAndPosition.has_value(); }
};

/// General error class, optionally carrying a source location.
class Error : public std::runtime_error {
  std::optional<Location> location;

  static std::optional<Location> wrap(const Location &location) {
    return location.isNull() ? std::nullopt : std::optional<Location>(location);
  }

public:
  Error(const std::string &what) : std::runtime_error(what) {}
  Error(const char *what) : std::runtime_error(what) {}
  Error(Location location, const std::string &what)
      : std::runtime_error(what), location(wrap(location)) {}
  Error(Location location, const char *what)
      : std::runtime_error(what), location(wrap(location)) {}
  bool hasLocation() const { return location.has_value(); }
  const Location &getLocation() const { return *location; }
};

} // End namespace hexutil

#endif // UTIL_HPP
