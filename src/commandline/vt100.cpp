#include"commandline/vt100.hpp"

namespace fetch {
namespace commandline {
namespace VT100 {

  
  std::map<std::string, uint16_t> const color_map = {
    {"black", 0},   {"red", 1},  {"green", 2}, {"yellow", 3},  {"blue", 4},
    {"magenta", 5}, {"cyan", 6}, {"white", 7}, {"default", 9}, {"0", 0},
    {"1", 1},       {"2", 2},    {"3", 3},     {"4", 4},       {"5", 5},
    {"6", 6},       {"7", 7},    {"9", 9}};

  uint16_t ColorFromString(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (color_map.find(name) == color_map.end()) return 9;
    return color_map.find(name)->second;
  }

  std::string GetColor(int const &fg, int const &bg) {
    std::stringstream ret("");
    ret << "\33[3" << fg << ";4" << bg << "m";
    return ret.str();
  }

  std::string GetColor(std::string const &f, std::string const &b) {
    uint16_t fg = ColorFromString(f);
    uint16_t bg = ColorFromString(b);
    std::stringstream ret("");
    ret << "\33[3" << fg << ";4" << bg << "m";
    return ret.str();
  }


  char const *Bold = "\33[1m";
  char const *Return = "\r";

  
};
};
};
