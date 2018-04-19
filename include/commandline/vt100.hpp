#ifndef VT100_HPP
#define VT100_HPP

#include <map>
#include <sstream>
#include <string>
#include <algorithm>

namespace fetch {
namespace commandline {
namespace VT100 {
  
  uint16_t ColorFromString(std::string name);
  std::string GetColor(int const &fg, int const &bg);
  std::string GetColor(std::string const &f, std::string const &b);

  inline static constexpr const char *DefaultAttributes() { return "\33[0m"; }
  inline static constexpr const char *ClearScreen() { return "\33[2J"; }
  extern char const *Bold;
  extern char const *Return;
  static const std::string Goto(uint16_t x, uint16_t y);

  static const std::string Down(uint16_t y);
  
  static const std::string Up(uint16_t y);
  static const std::string Right(uint16_t y);
  static const std::string Left(uint16_t y);
};
};
};
#endif
