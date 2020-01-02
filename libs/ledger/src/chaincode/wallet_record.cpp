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

#include "ledger/chaincode/wallet_record.hpp"
#include "variant/variant.hpp"

using fetch::byte_array::ConstByteArray;
using fetch::variant::Variant;

namespace fetch {
namespace ledger {
namespace {

/**
 * Deserialises deed from Tx JSON data
 *
 * @details The deserialised `deed` data-member can be nullptr (non-initialised
 * `std::smart_ptr<Deed>`) in the case when the objective is to REMOVE the deed
 * (Tx data contain empty JSON (`signees` and `thresholds` are **NOT** present).
 *
 * @param data Variant type object deserialised from Tx JSON data, which is
 * expected to contain the deed definition. This variant object will be used
 * for deserialisation to `Deed` structure.
 *
 * @return true if deserialisation passed successfully, false otherwise.
 */
bool DeedFromVariant(Variant const &variant_deed, DeedPtr &deed)
{
  // Provided variant is required to be either of Object type or Null to clearly
  // indicate the intention.
  if (!variant_deed.IsObject() && !variant_deed.IsNull())
  {
    return false;
  }

  auto const num_of_items_in_deed = variant_deed.size();
  if (num_of_items_in_deed == 0)
  {
    // This indicates request to REMOVE the deed (only `address` field has been
    // provided).
    deed.reset();
    return true;
  }

  if (num_of_items_in_deed != 2)
  {
    // This is INVALID attempt to AMEND the deed. Input deed variant is structurally
    // unsound for amend operation because it does NOT contain exactly 2 expected
    // elements (`signees` and `thresholds`).
    return false;
  }

  auto const v_thresholds{variant_deed[THRESHOLDS_NAME]};
  if (!v_thresholds.IsObject())
  {
    return false;
  }

  Deed::OperationTresholds thresholds;
  v_thresholds.IterateObject(
      [&thresholds](ConstByteArray const &operation, Variant const &v_threshold) -> bool {
        thresholds[operation] = v_threshold.As<Deed::Weight>();
        return true;
      });

  auto const v_signees{variant_deed[SIGNEES_NAME]};
  if (!v_signees.IsObject())
  {
    return false;
  }

  Deed::Signees signees;
  v_signees.IterateObject(
      [&signees](ConstByteArray const &display_address, Variant const &v_weight) -> bool {
        chain::Address address{};
        if (chain::Address::Parse(display_address, address))
        {
          signees[address] = v_weight.As<Deed::Weight>();
        }

        return true;
      });

  deed = std::make_shared<Deed>(std::move(signees), std::move(thresholds));
  return true;
}

Variant DeedToVariant(DeedPtr const &deed)
{
  auto v_deed{Variant::Object()};

  if (!deed)
  {
    return v_deed;
  }

  if (!deed->signees().empty())
  {
    auto &v_signees = v_deed[SIGNEES_NAME] = Variant::Object();
    for (auto const &signee : deed->signees())
    {
      v_signees[signee.first.display()] = signee.second;
    }
  }

  if (!deed->operation_thresholds().empty())
  {
    auto &v_thresholds = v_deed[THRESHOLDS_NAME] = Variant::Object();
    for (auto const &threshold : deed->operation_thresholds())
    {
      v_thresholds[threshold.first] = threshold.second;
    }
  }

  return v_deed;
}

}  // namespace

// deed entry constants
extern ConstByteArray const ADDRESS_NAME{"address"};
extern ConstByteArray const FROM_NAME{"from"};
extern ConstByteArray const TO_NAME{"to"};
extern ConstByteArray const AMOUNT_NAME{"amount"};
extern ConstByteArray const THRESHOLDS_NAME{"thresholds"};
extern ConstByteArray const SIGNEES_NAME{"signees"};

/**
 * Deserialises deed from Tx JSON data
 *
 * @details The deserialised `deed` data-member can be nullptr (non-initialised
 * `std::smart_ptr<Deed>`) in the case when the objective is to REMOVE the deed
 * (when Tx data contains empty JSON dictionary).
 *
 * @param data Variant type object deserialised from Tx JSON data, which is
 * expected to contain the deed definition. This variant object will be used
 * for deserialisation to `Deed` structure.
 *
 * @return true if deserialisation passed successfully, false otherwise.
 */
bool WalletRecord::CreateDeed(Variant const &data)
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

variant::Variant WalletRecord::ExtractDeed() const
{
  return DeedToVariant(deed);
}

void WalletRecord::CollectStake(uint64_t block_index)
{
  // Point to stake equal to or greater than block index
  auto stop_point = cooldown_stake.lower_bound(block_index);
  auto it         = cooldown_stake.begin();

  // Iterate upwards collecting stake
  while (it != cooldown_stake.end() && it != stop_point)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Entity is collecting cooled down stake of value: ", it->second);
    balance += it->second;
    it = cooldown_stake.erase(it);
  }
}

}  // namespace ledger
}  // namespace fetch
