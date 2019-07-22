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
#include <map>

// TODO(HUT): doesn't putting this here pollute the namespace?
using fetch::variant::Variant;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromBase64;

namespace fetch {
namespace ledger {

byte_array::ConstByteArray const ADDRESS_NAME{"address"};
byte_array::ConstByteArray const FROM_NAME{"from"};
byte_array::ConstByteArray const TO_NAME{"to"};
byte_array::ConstByteArray const AMOUNT_NAME{"amount"};
byte_array::ConstByteArray const TRANSFER_NAME{"transfer"};
byte_array::ConstByteArray const STAKE_NAME{"stake"};
byte_array::ConstByteArray const AMEND_NAME{"amend"};
byte_array::ConstByteArray const THRESHOLDS_NAME{"thresholds"};
byte_array::ConstByteArray const SIGNEES_NAME{"signees"};

using DeedShrdPtr = std::shared_ptr<Deed>;

/**
 * Deserialises deed from Tx JSON data
 *
 * @details The deserialised `deed` data-member can be nullptr (non-initialised
 * `std::smart_ptr<Deed>`) in the case when the objective is to REMOVE the deed
 * (Tx JSON data contain only `address` element, but `signees` and `thresholds`
 * are **NOT** present).
 *
 * @param data Variant type object deserialised from Tx JSON data, which is
 * expected to contain the deed definition. This variant object will be used
 * for deserialisation to `Deed` structure.
 *
 * @return true if deserialisation passed successfully, false otherwise.
 */
bool DeedFromVariant(Variant const &variant_deed, DeedShrdPtr &deed)
{
  auto const num_of_items_in_deed = variant_deed.size();
  if (num_of_items_in_deed == 1 && variant_deed.Has(ADDRESS_NAME))
  {
    // This indicates request to REMOVE the deed (only `address` field has been
    // provided).
    deed.reset();
    return true;
  }
  else if (num_of_items_in_deed != 3)
  {
    // This is INVALID attempt to AMEND the deed. Input deed variant is structurally
    // unsound for amend operation because it does NOT contain exactly 3 expected
    // elements (`address`, `signees` and `thresholds`).
    return false;
  }

  auto const v_thresholds{variant_deed[THRESHOLDS_NAME]};
  if (!v_thresholds.IsObject())
  {
    return false;
  }

  Deed::OperationTresholds thresholds;
  v_thresholds.IterateObject([&thresholds](byte_array::ConstByteArray const &operation,
                                           variant::Variant const &          v_threshold) -> bool {
    thresholds[operation] = v_threshold.As<Deed::Weight>();
    return true;
  });

  auto const v_signees{variant_deed[SIGNEES_NAME]};
  if (!v_signees.IsObject())
  {
    return false;
  }

  Deed::Signees signees;
  v_signees.IterateObject([&signees](byte_array::ConstByteArray const &display_address,
                                     variant::Variant const &          v_weight) -> bool {
    Address address{};
    if (Address::Parse(display_address, address))
    {
      signees[address] = v_weight.As<Deed::Weight>();
    }

    return true;
  });

  deed = std::make_shared<Deed>(std::move(signees), std::move(thresholds));
  return true;
}

/* Implements a record to store wallet contents. */
struct WalletRecord
{
  // Map of block number stake will be released on to amount to release
  using CooldownStake = std::map<uint64_t, uint64_t>;

  uint64_t              balance{0};
  uint64_t              stake{0};
  CooldownStake         cooldown_stake;
  std::shared_ptr<Deed> deed;

  /**
   * Deserialises deed from Tx JSON data
   *
   * @details The deserialised `deed` data-member can be nullptr (non-initialised
   * `std::smart_ptr<Deed>`) in the case when the objective is to REMOVE the deed
   * (Tx JSON data contain only `address` element, but `signees` and `thresholds`
   * are **NOT** present).
   *
   * @param data Variant type object deserialised from Tx JSON data, which is
   * expected to contain the deed definition. This variant object will be used
   * for deserialisation to `Deed` structure.
   *
   * @return true if deserialisation passed successfully, false otherwise.
   */
  bool CreateDeed(Variant const &data)
  {
    if (!DeedFromVariant(data, deed))
    {
      // Invalid format of deed json data from Tx.
      return false;
    }

    if (!deed)
    {
      // Valid case - the deed is **NOT** present **INTENTIONALLY**.
      return true;
    }

    if (deed->IsSane())
    {
      return true;
    }

    deed.reset();
    return false;
  }

  void CollectStake(uint64_t block_index)
  {
    // Point to stake equal to or greater than block index
    auto stop_point = cooldown_stake.lower_bound(block_index);
    auto it         = cooldown_stake.begin();

    // Iterate upwards collecting stake
    while (it != cooldown_stake.end() && it != stop_point)
    {
      balance += it->second;
      it = cooldown_stake.erase(it);
    }
  }
};

}  // namespace ledger

namespace serializers {
template <typename D>
struct ArraySerializer<ledger::WalletRecord, D>
{
public:
  using Type       = ledger::WalletRecord;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &b)
  {
    auto array = array_constructor(b.deed ? 2 : 1);
    array.Append(b.balance);
    if (b.deed)
    {
      array.Append(*b.deed);
    }
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &b)
  {
    array.GetNextValue(b.balance);
    if (array.size() == 2)
    {
      if (!b.deed)
      {
        b.deed.reset(new ledger::Deed{});
      }
      array.GetNextValue(*b.deed);
    }
  }
};
}  // namespace serializers

}  // namespace fetch
