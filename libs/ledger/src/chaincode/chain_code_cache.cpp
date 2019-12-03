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

namespace {

using byte_array::ConstByteArray;

template <std::size_t MIN_LENGTH, std::size_t MAX_LENGTH>
bool IsBase58(ConstByteArray const &value)
{
  char const *buffer = value.char_pointer();

  if (!(value.size() >= MIN_LENGTH && value.size() <= MAX_LENGTH))
  {
    return false;
  }

  for (std::size_t i = 0; i < value.size(); ++i, ++buffer)
  {
    // 1-9A-HJ-NP-Za-km-z
    bool const valid =
        ((('1' <= *buffer) && ('9' >= *buffer)) || (('A' <= *buffer) && ('H' >= *buffer)) ||
         (('J' <= *buffer) && ('N' >= *buffer)) || (('P' <= *buffer) && ('Z' >= *buffer)) ||
         (('a' <= *buffer) && ('k' >= *buffer)) || (('m' <= *buffer) && ('z' >= *buffer)));

    if (!valid)
    {
      return false;
    }
  }

  return true;
}

bool IsIdentity(ConstByteArray const &value)
{
  return IsBase58<48, 50>(value);
}

}  // namespace

ChainCodeCache::ContractPtr ChainCodeCache::Lookup(ContractId const &contract_id,
                                                   StorageInterface &storage)
{
  // attempt to locate the contract in the cache
  ContractPtr contract = FindInCache(contract_id);

  // if this fails create the contract
  if (!contract)
  {
    if (IsIdentity(contract_id))
    {
      contract = CreateSmartContract<SmartContract>(contract_id, storage);
    }
    else
    {
      contract = CreateChainCode(contract_id);
    }

    assert(static_cast<bool>(contract));

    // update the cache
    cache_.emplace(contract_id, contract);
  }

  // periodically run cache maintenance
  if ((++counter_ & CLEANUP_MASK) == 0)
  {
    RunMaintenance();
  }

  return contract;
}

ChainCodeCache::ContractPtr ChainCodeCache::FindInCache(ContractId const &contract_id)
{
  ContractPtr contract;

  // attempt to look up the contract in the cache
  auto it = cache_.find(contract_id);
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
