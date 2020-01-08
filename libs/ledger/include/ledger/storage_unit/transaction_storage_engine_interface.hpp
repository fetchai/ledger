#pragma once
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

#include "chain/transaction.hpp"
#include "chain/transaction_layout.hpp"
#include "core/digest.hpp"

namespace fetch {
namespace ledger {

class TransactionStorageEngineInterface
{
public:
  using TxArray   = std::vector<chain::Transaction>;
  using TxLayouts = std::vector<chain::TransactionLayout>;

  // Construction / Destruction
  TransactionStorageEngineInterface()          = default;
  virtual ~TransactionStorageEngineInterface() = default;

  /// @name Transaction Storage Interface
  /// @{

  /**
   * Add a new transaction to the storage engine
   *
   * @param tx The transaction to be stored
   * @param is_recent flag to signal if it is a recently seen transaction
   */
  virtual void Add(chain::Transaction const &tx, bool is_recent) = 0;

  /**
   * Query if a transaction is present in the storage engine
   *
   * @param tx_digest The digest of the transaction being queried
   * @return true if the transaction is present, otherwise false
   */
  virtual bool Has(Digest const &tx_digest) const = 0;

  /**
   * Retrieve a transaction from the storage engine
   *
   * @param tx_digest The digest of the transaction being queried
   * @param tx The output transaction to be populated
   * @return true if successful, otherwise false
   */
  virtual bool Get(Digest const &tx_digest, chain::Transaction &tx) const = 0;

  /**
   * Get the total number of stored transactions in this storage engine
   *
   * @return The total number of transactions
   */
  virtual std::size_t GetCount() const = 0;

  /**
   * Confirm that a transaction should be kept (has been included in the block chain)
   *
   * @param tx_digest The digest of the transaction being confirmed
   */
  virtual void Confirm(Digest const &tx_digest) = 0;

  /**
   * Get a set of transaction layouts corresponding to the most recent transactions received
   *
   * @param max_to_poll The maximum number of transactions be returned
   * @return The set of transaction layouts for the most recent transactions seen
   */
  virtual TxLayouts GetRecent(uint32_t max_to_poll) = 0;

  virtual TxArray PullSubtree(Digest const &partial_digest, uint64_t bit_count,
                              uint64_t pull_limit) = 0;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
