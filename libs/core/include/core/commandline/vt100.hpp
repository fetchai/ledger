#pragma once
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

#include <cstdint>
#include <sstream>
#include <string>

namespace fetch {
namespace commandline {
namespace VT100 {

uint16_t    ColorFromString(std::string name);
std::string GetColor(int const &fg, int const &bg);
std::string GetColor(std::string const &f, std::string const &b);

inline static constexpr const char *DefaultAttributes()
{
#ifdef FETCH_DISABLE_COLOUR_LOG_OUTPUT
  return "";
#else   // !FETCH_DISABLE_COLOUR_LOG_OUTPUT
  return "\33[0m";
#endif  // FETCH_DISABLE_COLOUR_LOG_OUTPUT
}
inline static constexpr const char *ClearScreen()
{
  return "\33[2J";
}
extern char const *Bold;
extern char const *Return;

inline static const std::string Goto(uint16_t x, uint16_t y)
{
  std::ostringstream ret;
  ret << "\33[" << y << ";" << x << "H";
  return ret.str();
}

inline static const std::string Down(uint16_t y)
{
  std::ostringstream ret;
  ret << "\33[" << y << "B";
  return ret.str();
}
inline static const std::string Up(uint16_t y)
{
  std::ostringstream ret;
  ret << "\33[" << y << "A";
  return ret.str();
}
inline static const std::string Right(uint16_t y)
{
  std::ostringstream ret;
  ret << "\33[" << y << "C";
  return ret.str();
}
inline static const std::string Left(uint16_t y)
{
  std::ostringstream ret;
  ret << "\33[" << y << "D";
  return ret.str();
}
}  // namespace VT100
}  // namespace commandline
}  // namespace fetch
