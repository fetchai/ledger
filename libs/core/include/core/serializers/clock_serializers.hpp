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

#include "core/serializers/main_serializer.hpp"

namespace fetch {
namespace serializers {

template <typename D>
struct ForwardSerializer<moment::ClockInterface::Duration, D>
{
public:
  using Type       = moment::ClockInterface::Duration;
  using DriverType = D;

  template <typename Serializer>
  static void Serialize(Serializer &serializer, Type const &item)
  {
    serializer << static_cast<uint64_t>(item.count());
  }

  template <typename Serializer>
  static void Deserialize(Serializer &deserializer, Type &item)
  {
    uint64_t time;
    deserializer >> time;
    item = Type(time);
  }
};

template <typename D>
struct ForwardSerializer<moment::ClockInterface::Timestamp, D>
{
public:
  using Type       = moment::ClockInterface::Timestamp;
  using DriverType = D;

  template <typename Serializer>
  static void Serialize(Serializer &serializer, Type const &item)
  {
    auto tse = item.time_since_epoch();
    serializer << tse;
  }

  template <typename Serializer>
  static void Deserialize(Serializer &deserializer, Type &item)
  {
    moment::ClockInterface::Duration tse;
    deserializer >> tse;
    item = Type(tse);
  }
};

}  // namespace serializers
}  // namespace fetch

