#pragma once
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

#include "crypto/mcl_dkg.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace fetch {
namespace beacon {
struct PublicKeyMessage
{
  uint64_t               round{0};
  crypto::mcl::PublicKey group_public_key;

  bool operator<(PublicKeyMessage const &other) const
  {
    // Lower rounds come first, hence >
    return round > other.round;
  }
};
}  // namespace beacon

namespace serializers {

template <typename D>
struct MapSerializer<beacon::PublicKeyMessage, D>
{
public:
  using Type       = beacon::PublicKeyMessage;
  using DriverType = D;

  static uint8_t const ROUND            = 0;
  static uint8_t const GROUP_PUBLIC_KEY = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &vv)
  {
    auto map = map_constructor(2);

    map.Append(ROUND, vv.round);
    map.Append(GROUP_PUBLIC_KEY, vv.group_public_key.getStr());
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &vv)
  {
    map.ExpectKeyGetValue(ROUND, vv.round);
    std::string key_str;
    map.ExpectKeyGetValue(GROUP_PUBLIC_KEY, key_str);
    vv.group_public_key.setStr(key_str);
  }
};

}  // namespace serializers
}  // namespace fetch
