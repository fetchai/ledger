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

#include "core/byte_array/byte_array.hpp"

namespace fetch {
namespace testing {

// Since the stacks do not serialize, use this class to make testing using strings easier
class StringProxy
{
public:
  StringProxy()
  {
    memset(this, 0, sizeof(decltype(*this)));
  }

  // Implicitly convert strings to this class
  StringProxy(std::string const &in)
  {
    memset(this, 0, sizeof(decltype(*this)));
    std::memcpy(string_as_chars, in.c_str(), std::min(in.size(), std::size_t(127)));
  }

  bool operator!=(StringProxy const &rhs) const
  {
    return memcmp(string_as_chars, rhs.string_as_chars, 128) != 0;
  }

  bool operator==(StringProxy const &rhs) const
  {
    return memcmp(string_as_chars, rhs.string_as_chars, 128) == 0;
  }

  char string_as_chars[128];
};

inline std::ostream &operator<<(std::ostream &os, StringProxy const &m)
{
  return os << m.string_as_chars;
}


}  // namespace testing
}  // namespace fetch
