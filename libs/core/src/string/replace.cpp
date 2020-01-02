//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/string/replace.hpp"

#include <algorithm>
#include <string>

namespace fetch {
namespace string {

std::string Replace(std::string value, char before, char after)
{
  std::replace(value.begin(), value.end(), before, after);
  return value;
}

bool Replace(std::string &orig, std::string const &what, std::string const &with)
{
  auto start{orig.find(what)};

  if (start == std::string::npos)
  {
    return false;
  }

  orig.replace(start, what.size(), with);

  return true;
}

}  // namespace string
}  // namespace fetch
