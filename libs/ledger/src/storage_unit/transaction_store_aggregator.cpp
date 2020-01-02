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

#include "ledger/storage_unit/transaction_store_aggregator.hpp"

namespace fetch {
namespace ledger {

/**
 * Create an instance of the transaction store aggregator
 *
 * @param pool The reference to the pool
 * @param store The reference to the store
 */
TransactionStoreAggregator::TransactionStoreAggregator(TransactionStoreInterface &pool,
                                                       TransactionStoreInterface &store)
  : pool_{pool}
  , store_{store}
{}

/**
 * Add a transaction to the store
 *
 * @param tx The transaction to set added to storage
 */
void TransactionStoreAggregator::Add(chain::Transaction const &tx)
{
  // transaction are always added to the pool first
  pool_.Add(tx);
}

/**
 * Check to see if requested transaction exists
 *
 * @param tx_digest The transaction digest to be searched for
 * @return true if present, otherwise false
 */
bool TransactionStoreAggregator::Has(Digest const &tx_digest) const
{
  return pool_.Has(tx_digest) || store_.Has(tx_digest);
}

/**
 * Lookup a transaction from the store
 *
 * @param tx_digest The transaction digest to lookup
 * @param tx
 * @return
 */
bool TransactionStoreAggregator::Get(Digest const &tx_digest, chain::Transaction &tx) const
{
  return pool_.Get(tx_digest, tx) || store_.Get(tx_digest, tx);
}

/**
 * Get the total number of transactions in this store
 *
 * @return The number of transactions stored
 */
uint64_t TransactionStoreAggregator::GetCount() const
{
  return pool_.GetCount() + store_.GetCount();
}

}  // namespace ledger
}  // namespace fetch
