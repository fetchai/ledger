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

#include "core/byte_array/byte_array.hpp"
#include "ledger/chain/transaction.hpp"
#include "storage/document.hpp"
#include "storage/resource_mapper.hpp"

#include <vector>

namespace fetch {
namespace ledger {

class StorageInterface
{
public:
  using Document        = storage::Document;
  using ResourceAddress = storage::ResourceAddress;
  using StateValue      = byte_array::ConstByteArray;

  /// @name State Interface
  /// @{
  virtual Document Get(ResourceAddress const &key)                          = 0;
  virtual Document GetOrCreate(ResourceAddress const &key)                  = 0;
  virtual void     Set(ResourceAddress const &key, StateValue const &value) = 0;
  virtual bool     Lock(ResourceAddress const &key)                         = 0;
  virtual bool     Unlock(ResourceAddress const &key)                       = 0;
  /// @}
};

class StorageUnitInterface : public StorageInterface
{
public:
  using Hash = byte_array::ConstByteArray;
  // using bookmark_type = uint64_t;  // TODO(issue 33): From keyvalue index

  using TransactionList = std::vector<Transaction>;
  using ConstByteArray  = byte_array::ConstByteArray;
  using TxSummaries     = std::vector<TransactionSummary>;

  // Construction / Destruction
  StorageUnitInterface()          = default;
  virtual ~StorageUnitInterface() = default;

  /// @name Transaction Interface
  /// @{
  virtual void AddTransaction(Transaction const &tx)                         = 0;
  virtual bool GetTransaction(ConstByteArray const &digest, Transaction &tx) = 0;

  virtual void AddTransactions(TransactionList const &txs)
  {
    for (auto const &tx : txs)
    {
      AddTransaction(tx);
    }
  }
  /// @}

  virtual TxSummaries PollRecentTx(uint32_t) = 0;

  /// @name Revertible Document Store Interface
  /// @{
  virtual Hash CurrentHash()                  = 0;
  virtual Hash LastCommitHash()               = 0;
  virtual bool RevertToHash(Hash const &hash) = 0;
  virtual Hash Commit()                       = 0;
  virtual bool HashExists(Hash const &hash)   = 0;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
