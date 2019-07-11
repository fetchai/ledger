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

#include "settings/setting.hpp"
#include "core/containers/is_in.hpp"

#include <istream>
#include <unordered_set>

namespace fetch {
namespace settings {
namespace {

const std::unordered_set<std::string> TRUE_VALUES = {"on", "1", "true", "enabled", "yes"};
const std::unordered_set<std::string> FALSE_VALUES = {"off", "0", "false", "disabled", "no"};

} // namespace

template <>
void Setting<bool>::FromStream(std::istream &stream)
{
  // extract the string value
  std::string value{};
  stream >> value;

  if (!stream.fail())
  {
    if (core::IsIn(TRUE_VALUES, value))
    {
      value_  = true;
    }
    else if (core::IsIn(FALSE_VALUES, value))
    {
      value_  = false;
    }
  }
}

template <>
void Setting<bool>::ToStream(std::ostream &stream) const
{
  stream << ((value_) ? "Yes" : "No");
}

} // namespace settings
} // namespace fetch
