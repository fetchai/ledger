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

#include "ledger/chaincode/cache.hpp"
#include "ledger/chaincode/factory.hpp"
#include "meta/is_log2.hpp"

#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <utility>

namespace fetch {
namespace ledger {

ChainCodeCache::ContractPtr ChainCodeCache::Lookup(byte_array::ConstByteArray const &contract_name)
{
  // attempt to locate the contract in the cache
  ContractPtr contract = FindInCache(contract_name);

  // if this fails create the contract
  if (!contract)
  {
    contract = CreateContract(contract_name);
  }

  // periodically run cache maintenance
  if ((++counter_ & CLEANUP_MASK) == 0)
  {
    RunMaintenance();
  }

  return contract;
}

ChainCodeCache::ContractPtr ChainCodeCache::FindInCache(byte_array::ConstByteArray const &name)
{
  ContractPtr contract;

  // attempt to lookup the contract in the cache
  auto it = cache_.find(name);
  if (it != cache_.end())
  {

    // extract the contract and refresh the cache timestamp
    contract             = it->second.chain_code;
    it->second.timestamp = Clock::now();
  }

  return contract;
}

ChainCodeCache::ContractPtr ChainCodeCache::CreateContract(byte_array::ConstByteArray const &name)
{
  ContractPtr contract = factory_.Create(name);

  // update the cache
  cache_.emplace(name, contract);

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
