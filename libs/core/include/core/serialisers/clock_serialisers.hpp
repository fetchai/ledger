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

#include "core/serialisers/main_serialiser.hpp"
#include "moment/clock_interfaces.hpp"

namespace fetch {
namespace serialisers {

template <typename D>
struct ForwardSerialiser<fetch::moment::ClockInterface::Duration, D>
{
public:
  using Type       = fetch::moment::ClockInterface::Duration;
  using DriverType = D;

  template <typename Serialiser>
  static void Serialise(Serialiser &serialiser, Type const &item)
  {
    serialiser << static_cast<uint64_t>(item.count());
  }

  template <typename Serialiser>
  static void Deserialise(Serialiser &deserialiser, Type &item)
  {
    uint64_t time;
    deserialiser >> time;
    item = Type(time);
  }
};

template <typename D>
struct ForwardSerialiser<moment::ClockInterface::Timestamp, D>
{
public:
  using Type       = moment::ClockInterface::Timestamp;
  using DriverType = D;

  template <typename Serialiser>
  static void Serialise(Serialiser &serialiser, Type const &item)
  {
    auto tse = item.time_since_epoch();
    serialiser << tse;
  }

  template <typename Serialiser>
  static void Deserialise(Serialiser &deserialiser, Type &item)
  {
    moment::ClockInterface::Duration tse;
    deserialiser >> tse;
    item = Type(tse);
  }
};

}  // namespace serialisers
}  // namespace fetch
