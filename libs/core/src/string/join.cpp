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

#include "core/string/join.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace fetch {
namespace core {

std::string Join(std::vector<std::string> const &segments, std::string const &strut)
{
  if (segments.empty())
  {
    return "";
  }

  std::ostringstream output;
  output << segments[0];

  for (std::size_t i = 1; i < segments.size(); ++i)
  {
    output << strut << segments[i];
  }

  return output.str();
}

}  // namespace core
}  // namespace fetch
