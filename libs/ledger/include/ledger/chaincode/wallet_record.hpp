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
extern byte_array::ConstByteArray const TRANSFER_NAME;
extern byte_array::ConstByteArray const STAKE_NAME;
extern byte_array::ConstByteArray const AMEND_NAME;
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
struct ArraySerializer<ledger::WalletRecord, D>
{
public:
  // TODO(issue 1426): Change this serializer to map
  using Type       = ledger::WalletRecord;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &b)
  {
    auto array = array_constructor(b.deed ? 4 : 3);
    array.Append(b.balance);
    if (b.deed)
    {
      array.Append(*b.deed);
    }

    array.Append(b.stake);
    array.Append(b.cooldown_stake);
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &b)
  {
    array.GetNextValue(b.balance);
    if (array.size() == 4)
    {
      if (!b.deed)
      {
        b.deed = std::make_shared<ledger::Deed>();
      }
      array.GetNextValue(*b.deed);
    }

    array.GetNextValue(b.stake);
    array.GetNextValue(b.cooldown_stake);
  }
};
}  // namespace serializers

}  // namespace fetch
