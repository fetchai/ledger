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
#include <string>

namespace fetch {
namespace muddle {

class NetworkId
{
public:
  using UnderlyingType = uint32_t;

  // Construction / Destruction
  NetworkId() = default;
  explicit NetworkId(char const id[4]);
  explicit NetworkId(UnderlyingType value);
  NetworkId(NetworkId const &) = default;
  ~NetworkId() = default;

  UnderlyingType const &value() const;
  std::string ToString() const;

  // Operators
  NetworkId &operator=(NetworkId const &) = default;

private:

  UnderlyingType value_{0};
};

inline NetworkId::NetworkId(UnderlyingType value)
  : value_{value}
{
}

inline NetworkId::UnderlyingType const &NetworkId::value() const
{
  return value_;
}


} // namespace muddle
} // namespace fetch