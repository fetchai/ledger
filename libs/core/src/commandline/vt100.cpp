//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/commandline/vt100.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <utility>

namespace fetch {
namespace commandline {
namespace VT100 {

std::map<std::string, uint16_t> const color_map = {
    {"black", 0}, {"red", 1},   {"green", 2},   {"yellow", 3}, {"blue", 4}, {"magenta", 5},
    {"cyan", 6},  {"white", 7}, {"default", 9}, {"0", 0},      {"1", 1},    {"2", 2},
    {"3", 3},     {"4", 4},     {"5", 5},       {"6", 6},      {"7", 7},    {"9", 9}};

uint16_t ColorFromString(std::string name)
{
  std::transform(name.begin(), name.end(), name.begin(),
                 [](char c) { return static_cast<char>(std::tolower(c)); });
  if (color_map.find(name) == color_map.end())
  {
    return 9;
  }
  return color_map.find(name)->second;
}

std::string GetColor(int const &fg, int const &bg)
{
#ifdef FETCH_DISABLE_COLOUR_LOG_OUTPUT
  return {};
#else   // !FETCH_DISABLE_COLOUR_LOG_OUTPUT
  std::stringstream ret("");
  ret << "\33[3" << fg << ";4" << bg << "m";
  return ret.str();
#endif  // FETCH_DISABLE_COLOUR_LOG_OUTPUT
}

std::string GetColor(std::string const &f, std::string const &b)
{
#ifdef FETCH_DISABLE_COLOUR_LOG_OUTPUT
  return {};
#else   // !FETCH_DISABLE_COLOUR_LOG_OUTPUT
  uint16_t          fg = ColorFromString(f);
  uint16_t          bg = ColorFromString(b);
  std::stringstream ret("");
  ret << "\33[3" << fg << ";4" << bg << "m";
  return ret.str();
#endif  // FETCH_DISABLE_COLOUR_LOG_OUTPUT
}

char const *Bold   = "\33[1m";
char const *Return = "\r";

}  // namespace VT100
}  // namespace commandline
}  // namespace fetch
