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

#include "core/byte_array/byte_array.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/main_serializer.hpp"
#include "muddle/address.hpp"

namespace fetch {
namespace messenger {

struct NetworkLocation
{
  using Address = muddle::Address;

  Address node;
  Address messenger;

  bool operator==(NetworkLocation const &other) const
  {
    return (node == other.node) && (messenger == other.messenger);
  }

  bool operator<(NetworkLocation const &other) const
  {
    return messenger < other.messenger;
  }
};

}  // namespace messenger

namespace serializers {

template <typename D>
struct MapSerializer<messenger::NetworkLocation, D>
{
public:
  using Type       = messenger::NetworkLocation;
  using DriverType = D;

  static uint8_t const NODE      = 1;
  static uint8_t const MESSENGER = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &location)
  {
    auto map = map_constructor(2);
    map.Append(NODE, location.node);
    map.Append(MESSENGER, location.messenger);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &location)
  {
    map.ExpectKeyGetValue(NODE, location.node);
    map.ExpectKeyGetValue(MESSENGER, location.messenger);
  }
};

}  // namespace serializers
}  // namespace fetch
