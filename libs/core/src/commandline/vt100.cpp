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

}  // namespace VT100
}  // namespace commandline
}  // namespace fetch
