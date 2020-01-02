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

#include "core/serializers/map_interface.hpp"

#include <cstdint>

namespace fetch {
namespace muddle {

struct RoutingMessage
{
  enum class Type
  {
    PING = 0,
    PONG,
    ROUTING_REQUEST,
    ROUTING_ACCEPTED,
    DISCONNECT_REQUEST,

    MAX_NUM_TYPES
  };

  Type type{Type::PING};
};

}  // namespace muddle

namespace serializers {

template <typename D>
struct MapSerializer<muddle::RoutingMessage, D>
{
public:
  using Type       = muddle::RoutingMessage;
  using DriverType = D;
  using EnumType   = uint64_t;

  static const uint8_t TYPE = 1;

  template <typename T>
  static void Serialize(T &map_constructor, Type const &msg)
  {
    auto map = map_constructor(1);
    map.Append(TYPE, static_cast<EnumType>(msg.type));
  }

  template <typename T>
  static void Deserialize(T &map, Type &msg)
  {
    static constexpr auto MAX_TYPE_VALUE = static_cast<EnumType>(Type::Type::MAX_NUM_TYPES);

    EnumType raw_type{0};
    map.ExpectKeyGetValue(TYPE, raw_type);

    // validate the type enum
    if (raw_type >= MAX_TYPE_VALUE)
    {
      throw std::runtime_error("Invalid type value");
    }

    msg.type = static_cast<Type::Type>(raw_type);
  }
};

}  // namespace serializers
}  // namespace fetch
