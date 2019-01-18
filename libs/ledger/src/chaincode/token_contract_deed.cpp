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

#include "ledger/chaincode/token_contract_deed.hpp"
#include "ledger/chain/transaction.hpp"

namespace fetch {
namespace ledger {
namespace {

Weight SigneesFullWeight(Deed::Signees const &signees)
{
  Weight full_weight = 0;
  for (auto const &signee : signees)
  {
    full_weight += signee.second;
  }
  return full_weight;
}

}  // namespace

Deed::Deed(Signees signees, OperationTresholds thresholds)
  : signees_{std::move(signees)}
  , operationTresholds_{std::move(thresholds)}
  , full_weight_{SigneesFullWeight(signees_)}
{}

bool Deed::IsSane() const
{
  for (auto const &opTreshold : operationTresholds_)
  {
    if (opTreshold.second > full_weight_)
    {
      return false;
    }
  }
  return true;
}

bool Deed::Verify(chain::VerifiedTransaction const &tx, DeedOperation const &operation) const
{
  auto const op = operationTresholds_.find(operation);
  if (op == operationTresholds_.end())
  {
    // Unknown operation
    return false;
  }

  Weight vote = 0;
  for (auto const &signee : signees_)
  {
    if (tx.signatures().count(crypto::Identity{signee.first}) > 0)
    {
      vote += signee.second;
    }
  }

  // Evaluating sufficiency of accumulated vote:
  return vote >= op->second;
}

}  // namespace ledger
}  // namespace fetch
