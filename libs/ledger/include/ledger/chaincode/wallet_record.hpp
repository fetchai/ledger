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

#include "core/byte_array/decoders.hpp"
#include "ledger/chaincode/deed.hpp"
#include "logging/logging.hpp"

#include <cstdint>
#include <map>
#include <memory>

namespace fetch {

namespace variant {
class Variant;
}

namespace ledger {

extern byte_array::ConstByteArray const ADDRESS_NAME;
extern byte_array::ConstByteArray const FROM_NAME;
extern byte_array::ConstByteArray const TO_NAME;
extern byte_array::ConstByteArray const AMOUNT_NAME;
extern byte_array::ConstByteArray const THRESHOLDS_NAME;
extern byte_array::ConstByteArray const SIGNEES_NAME;

/* Implements a record to store wallet contents. */
struct WalletRecord
{
  static constexpr char const *LOGGING_NAME = "WalletRecord";

  // Map of block number stake will be released on to amount to release
  using CooldownStake = std::map<uint64_t, uint64_t>;

  uint64_t      balance{0};
  uint64_t      stake{0};
  CooldownStake cooldown_stake;
  DeedPtr       deed;

  bool CreateDeed(variant::Variant const &data);
  void CollectStake(uint64_t block_index);
};

}  // namespace ledger

namespace serializers {

template <typename D>
struct MapSerializer<ledger::WalletRecord, D>
{
public:
  using Type       = ledger::WalletRecord;
  using DriverType = D;

  static uint8_t const BALANCE        = 1;
  static uint8_t const STAKE          = 2;
  static uint8_t const COOLDOWN_STAKE = 3;
  static uint8_t const DEED           = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &b)
  {
    auto map = map_constructor(b.deed ? 4 : 3);
    map.Append(BALANCE, b.balance);
    map.Append(STAKE, b.stake);
    map.Append(COOLDOWN_STAKE, b.cooldown_stake);

    if (b.deed)
    {
      map.Append(DEED, *b.deed);
    }
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &map, Type &b)
  {
    bool const has_deed = map.size() == 4;

    map.ExpectKeyGetValue(BALANCE, b.balance);
    map.ExpectKeyGetValue(STAKE, b.stake);
    map.ExpectKeyGetValue(COOLDOWN_STAKE, b.cooldown_stake);

    if (has_deed)
    {
      b.deed = std::make_shared<ledger::Deed>();

      map.ExpectKeyGetValue(DEED, *b.deed);
    }
  }
};

}  // namespace serializers

}  // namespace fetch
