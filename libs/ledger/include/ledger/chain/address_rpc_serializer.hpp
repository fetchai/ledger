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

#include "core/serializers/byte_array.hpp"
#include "ledger/chain/address.hpp"
#include "core/serializers/base_types.hpp"

namespace fetch {
namespace serializers {


template< typename D >
struct MapSerializer< ledger::Address, D > // TODO: Consider using forward to bytearray
{
public:
  using Type = ledger::Address;
  using DriverType = D;

  static uint8_t const ADDRESS = 1;

  template< typename Constructor >
  static void Serialize(Constructor & map_constructor, Type const & address)
  {
    auto map = map_constructor(1);
    map.Append(ADDRESS, address);
  }

  template< typename MapDeserializer >
  static void Deserialize(MapDeserializer & map, Type & address)
  {
    uint8_t key;
    byte_array::ConstByteArray data;  
    map.GetNextKeyPair(key, data);
    address = Type{data};
  }  
};


}  // namespace serializers
}  // namespace fetch
