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

#include "chain/common_types.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/main_serializer.hpp"

namespace fetch {

namespace ledger {

struct SmartContractWrapper
{
  using ConstByteArray = byte_array::ConstByteArray;

  SmartContractWrapper() = default;
  SmartContractWrapper(ConstByteArray source, uint64_t timestamp);

  // contract source
  ConstByteArray source;
  // metadata
  uint64_t creation_timestamp;
};

}  // namespace ledger

namespace serializers {

template <typename D>
struct MapSerializer<ledger::SmartContractWrapper, D>
{
public:
  using Type       = ledger::SmartContractWrapper;
  using DriverType = D;

  static uint8_t const SOURCE = 1;
  // metadata
  static uint8_t const CREATION_TIMESTAMP = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &o)
  {
    auto map = map_constructor(2);
    map.Append(SOURCE, o.source);
    map.Append(CREATION_TIMESTAMP, o.creation_timestamp);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &o)
  {
    map.ExpectKeyGetValue(SOURCE, o.source);
    map.ExpectKeyGetValue(CREATION_TIMESTAMP, o.creation_timestamp);
  }
};
}  // namespace serializers

}  // namespace fetch
