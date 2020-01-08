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
#include "messenger/network_location.hpp"
#include "muddle/address.hpp"

namespace fetch {
namespace messenger {

struct Message
{
  using ConstByteArray = byte_array::ConstByteArray;

  NetworkLocation from;
  NetworkLocation to;

  ConstByteArray protocol;
  ConstByteArray context;
  ConstByteArray payload;

  bool operator==(Message const &other) const
  {
    return (from == other.from) && (to == other.to) && (protocol == other.protocol) &&
           (payload == other.payload);
  }

  bool operator<(Message const &other) const
  {
    return from < other.from;
  }
};

}  // namespace messenger

namespace serializers {

template <typename D>
struct MapSerializer<messenger::Message, D>
{
public:
  using Type       = messenger::Message;
  using DriverType = D;

  static uint8_t const FROM     = 1;
  static uint8_t const TO       = 2;
  static uint8_t const PROTOCOL = 3;
  static uint8_t const CONTEXT  = 4;
  static uint8_t const PAYLOAD  = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &message)
  {
    auto map = map_constructor(5);
    map.Append(FROM, message.from);
    map.Append(TO, message.to);
    map.Append(PROTOCOL, message.protocol);
    map.Append(CONTEXT, message.context);
    map.Append(PAYLOAD, message.payload);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &message)
  {
    map.ExpectKeyGetValue(FROM, message.from);
    map.ExpectKeyGetValue(TO, message.to);
    map.ExpectKeyGetValue(PROTOCOL, message.protocol);
    map.ExpectKeyGetValue(CONTEXT, message.context);
    map.ExpectKeyGetValue(PAYLOAD, message.payload);
  }
};

}  // namespace serializers

}  // namespace fetch
