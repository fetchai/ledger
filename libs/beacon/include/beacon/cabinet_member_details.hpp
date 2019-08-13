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

#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/bls_base.hpp"
#include "crypto/bls_dkg.hpp"
#include "crypto/prover.hpp"

#include <memory>
#include <vector>

namespace fetch {
namespace beacon {

struct CabinetMemberDetails  // TODO: rename into CabinetMember
{
  using BeaconManager = dkg::BeaconManager;
  using Identity      = crypto::Identity;
  using Signature     = byte_array::ConstByteArray;

  /// Payload
  /// @{
  Identity          identity;
  BeaconManager::Id id;
  /// @}

  Signature signature;
};

}  // namespace beacon

namespace serializers {

template <typename D>
struct MapSerializer<beacon::CabinetMemberDetails, D>
{
public:
  using Type       = beacon::CabinetMemberDetails;
  using DriverType = D;

  static uint8_t const IDENTITY  = 0;
  static uint8_t const BEACON_ID = 1;
  static uint8_t const SIGNATURE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &member)
  {
    auto map = map_constructor(3);

    map.Append(IDENTITY, member.identity);
    map.Append(BEACON_ID, member.id);
    map.Append(SIGNATURE, member.signature);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &member)
  {
    map.ExpectKeyGetValue(IDENTITY, member.identity);
    map.ExpectKeyGetValue(BEACON_ID, member.id);
    map.ExpectKeyGetValue(SIGNATURE, member.signature);
  }
};

}  // namespace serializers
}  // namespace fetch
