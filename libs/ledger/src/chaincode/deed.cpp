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

#include "ledger/chaincode/deed.hpp"

#include "ledger/chain/transaction.hpp"

namespace fetch {
namespace ledger {
namespace {

/*
 * Below is expected deed JSON structure for CREATION of deed on provided
 * address, or AMENDing the deed currently existing on the provided address.
 * All 3 root elements `address`, `signees` and `thresholds` are MANDATORY,
 * `signees` MUST contain at LEAST one signee, and `thresholds` must contain
 * at LEAST one threshold. Where only known & handled thresholds are "transfer"
 * and "amend". The "amend" threshold is utilized for amending (what includes
 * DELETION) of the pre-existing deed.
 * PLEASE BE MINDFUL, that even if permitted, omission of the "amend" threshold
 * has severe consequences - forever DISABLING possibility to amend the deed!
 * {
 *   "address": "BASE64_ENCODED_DESTINATION_ADDRESS",
 *   "signees" : {
 *     "BASE64_ENCODED_ADDRESS_0" : VOTING_WEIGHT_0 as UNSIGNED_INT,
 *     "BASE64_ENCODED_ADDRESS_1" : VOTING_WEIGHT_1 as UNSIGNED_INT,
 *     ...,
 *     "BASE64_ENCODED_ADDRESS_N" : VOTING_WEIGHT_N as UNSIGNED_INT
 *     },
 *   "thresholds" : {
 *     "transfer" : UNSIGNED_INT,
 *     "amend" : UNSIGNED_INT
 *     }
 * }
 * @example Example:
 * {
 *   "address" : "OTc3NTY1MzAwMTMyMzQzMzM2Ng==",
 *   "signees" : {
 *     "NTAzOTA1MzM4NDM3OTM3MzI5Ng==" : 1,
 *     "OTc3NTY1MzAwMTMyMzQzMzM2Ng==" : 2,
 *     "ODc4MDQ4NDMxNDM1MzY4NDgwNg==" : 3
 *     },
 *   "thresholds" : {
 *     "transfer" : 4,
 *     "amend" : 6
 *     }
 * }
 *
 * Below is expected deed JSON structure for DELETION of the deed currently
 * existing on the provided address.
 * Both `signees` AND `thresholds` elements must NOT be present to clearly
 * indicate intention to DELETE the pre-existing deed - if any of these elements
 * are present, amend procedure will fail and so pre-existing deed will remain
 * in effect:
 * {
 *   "address": "BASE64_ENCODED_DESTINATION_ADDRESS"
 * }
 * @example
 * {
 *   "address": "MTM1NTgwMTI1ODkwMzQzNDMyODQ="
 * }
 *
 */

/**
 * Calculates the "full voting power" = sum of weights of ALL signees
 *
 * @return calculated full weight.
 */
inline Deed::Weight SigneesFullWeight(Deed::Signees const &signees)
{
  Deed::Weight full_weight = 0;
  for (auto const &signee : signees)
  {
    full_weight += signee.second;
  }
  return full_weight;
}
}  // namespace

Deed::Deed(Signees signees, OperationTresholds thresholds)
  : signees_{std::move(signees)}
  , operation_thresholds_{std::move(thresholds)}
  , full_weight_{SigneesFullWeight(signees_)}
{}

/**
 * Checks sanity of current deed state
 *
 * @details Deed is considered sane if **ALL** the following conditions are met:
 *   1) There is defined **AT LEAST** one signee,
 *   2) There is defined **AT LEAST** one threshold,
 *   3) Every signee (individually) must have weight **HIGHER** than zero,
 *   4) Every threshold (individually) must have value **HIGHER ** than zero,
 *   5) There is enough voting power (in `signees_` pool) to achieve **ANY**
 *   threshold defined in `operation_thresholds_` container.
 *
 * @return true if sanity check passes, false otherwise
 */
bool Deed::IsSane() const
{
  if (operation_thresholds_.size() == 0 || signees_.size() == 0)
  {
    return false;
  }

  for (auto const &signee : signees_)
  {
    if (signee.second == 0)
    {
      return false;
    }
  }

  for (auto const &threshold : operation_thresholds_)
  {
    if (threshold.second == 0 || threshold.second > full_weight_)
    {
      return false;
    }
  }
  return true;
}

/**
 * Verifies if transaction contains list of signatories necessary to reach the
 * threshold for selected operation.
 *
 * @details This verifies the transaction - meaning that there is ENOUGH
 * (verified) signatories provided in the Transaction such that they are
 * **AFFILIATED** to the requested operation **AND** sum of their voting weights
 * (weights are defined in deed by signees container) is **HIGH ENOUGH** to
 * reach the threshold for the requested operation(defined in deed by thresholds
 * container).
 *
 * @return true if sanity check passes, false otherwise
 */

// TODO(EJF): Rework to use signatories
bool Deed::Verify(Transaction const &tx, DeedOperation const &operation) const
{
  auto const op = operation_thresholds_.find(operation);
  if (op == operation_thresholds_.end())
  {
    // Unknown operation
    return false;
  }

  // cache all the addresses that are present in this current transaction
  using AddressSet = std::unordered_set<Address>;
  AddressSet tx_address_set{};
  for (auto const &signee : tx.signatories())
  {
    tx_address_set.insert(Address{signee.identity});
  }

  Weight vote = 0;
  for (auto const &signee : signees_)
  {
    bool const signee_present = tx_address_set.find(signee.first) != tx_address_set.end();

    // if this signee is present, then update the vote count
    if (signee_present)
    {
      vote += signee.second;
    }
  }

  // Evaluating sufficiency of accumulated vote:
  return vote >= op->second;
}

/**
 * Infers MANDATORY weights per each threshold (if such exists)
 *
 * @details Some of signees weights might be MANDATORY to be present in order to
 * accumulate enough voting power for reaching given threshold.
 * Definition of MANDATORY weights (for specific threshold value) is that they
 * are absolutely ESSENTIAL to participate on voting (sign the transaction) in
 * order to achieve the given threshold. **IF** any of such mandatory weight(s)
 * would be MISSING (= would NOT sign the TX), then it is NOT principally
 * possible to accumulate enough voting power from ALL REMAINING voters to reach
 * given threshold.
 *
 * @return matrix of mandatory weights. Matrix is container containing mapping
 * between threshold value and related container of MANDATORY weights defined as
 * mapping between mandatory weight and its count (how many of such weights are
 * mandatory).
 */
Deed::MandatorityMatrix Deed::InferMandatoryWeights() const
{
  Deed::MandatorityMatrix mandatory;

  for (auto const &t : operation_thresholds_)
  {
    auto const &threshold = t.second;
    auto const  overhead  = full_weight_ - threshold;

    Deed::Weights weights;
    for (auto const &s : signees_)
    {
      auto const &signee_weight = s.second;
      auto        it            = weights.find(signee_weight);

      if (it != weights.end())
      {
        auto &weight_count = it->second;
        ++weight_count;
        continue;
      }
      weights[signee_weight] = 1;
    }

    for (auto const &w : weights)
    {
      auto const &weight       = w.first;
      auto const &weight_count = w.second;

      auto const neccessary_count = overhead / weight;
      if (weight_count > neccessary_count)
      {
        mandatory[threshold][weight] = w.second - neccessary_count;
      }
    }
  }
  return mandatory;
}

}  // namespace ledger
}  // namespace fetch
