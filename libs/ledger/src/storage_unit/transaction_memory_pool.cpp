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
#include "ledger/storage_unit/transaction_memory_pool.hpp"

namespace fetch {
namespace ledger {

/**
 * Add a transaction to the store
 *
 * @param tx The transaction to set added to storage
 */
void TransactionMemoryPool::Add(chain::Transaction const &tx)
{
  FETCH_LOCK(lock_);
  transaction_store_[tx.digest()] = tx;
}

/**
 * Check to see if requested transaction exists
 *
 * @param tx_digest The transaction digest to be searched for
 * @return true if present, otherwise false
 */
bool TransactionMemoryPool::Has(Digest const &tx_digest) const
{
  FETCH_LOCK(lock_);
  return transaction_store_.find(tx_digest) != transaction_store_.end();
}

/**
 * Lookup a transaction from the store
 *
 * @param tx_digest The transaction digest to lookup
 * @param tx
 * @return true if successful, otherwise false
 */
bool TransactionMemoryPool::Get(Digest const &tx_digest, chain::Transaction &tx) const
{
  bool success{false};

  FETCH_LOCK(lock_);

  auto it = transaction_store_.find(tx_digest);
  if (it != transaction_store_.end())
  {
    tx      = it->second;
    success = true;
  }

  return success;
}

/**
 * Get the total number of transactions in this store
 *
 * @return The number of transactions stored
 */
uint64_t TransactionMemoryPool::GetCount() const
{
  FETCH_LOCK(lock_);
  return static_cast<uint64_t>(transaction_store_.size());
}

/**
 * Remove a transaction from the pool
 *
 * @param tx_digest The transaction being removed
 */
void TransactionMemoryPool::Remove(Digest const &tx_digest)
{
  FETCH_LOCK(lock_);
  transaction_store_.erase(tx_digest);
}

}  // namespace ledger
}  // namespace fetch
