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

#include "ledger/chaincode/chain_code_cache.hpp"
#include "ledger/chaincode/chain_code_factory.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/smart_contract.hpp"
#include "ledger/chaincode/smart_contract_factory.hpp"

#include <cassert>
#include <chrono>
#include <unordered_map>
#include <utility>

namespace fetch {
namespace ledger {

ChainCodeCache::ContractPtr ChainCodeCache::Lookup(Identifier const &contract_id,
                                                   StorageInterface &storage)
{
  // attempt to locate the contract in the cache
  ContractPtr contract = FindInCache(contract_id);

  // if this fails create the contract
  if (!contract)
  {
    if (contract_id.type() == Identifier::Type::SMART_OR_SYNERGETIC_CONTRACT)
    {
      auto const contract_digest = contract_id.qualifier().FromHex();
      contract                   = CreateSmartContract<SmartContract>(contract_digest, storage);
    }
    else
    {
      auto const &contract_name = contract_id.full_name();
      contract                  = CreateChainCode(contract_name);
    }

    // update the cache
    if (contract)
    {
      cache_.emplace(contract_id.qualifier(), contract);
    }
  }

  // periodically run cache maintenance
  if ((++counter_ & CLEANUP_MASK) == 0)
  {
    RunMaintenance();
  }

  return contract;
}

ChainCodeCache::ContractPtr ChainCodeCache::FindInCache(Identifier const &contract_id)
{
  ContractPtr contract;

  // attempt to look up the contract in the cache
  auto it = cache_.find(contract_id.qualifier());
  if (it != cache_.end())
  {
    // extract the contract and refresh the cache timestamp
    contract             = it->second.chain_code;
    it->second.timestamp = Clock::now();
  }

  return contract;
}

void ChainCodeCache::RunMaintenance()
{
  static const std::chrono::hours CACHE_LIFETIME{1};

  Timepoint const now = Clock::now();

  for (auto it = cache_.begin(), end = cache_.end(); it != end;)
  {
    auto const delta = now - it->second.timestamp;
    if (delta >= CACHE_LIFETIME)
    {
      it = cache_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

}  // namespace ledger
}  // namespace fetch
