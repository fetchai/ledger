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

#include "ledger/storage_unit/transaction_store_interface.hpp"

namespace fetch {
namespace ledger {

/**
 * The transaction store aggregate is an adapter presents two transactions the two store objects
 * as one. More concretely it is used to present a single transaction store interface to cover both
 * the transaction memory pool and the transaction archive.
 *
 * The transaction memory pool is always queried first, failure to find a transaction in pool causes
 * the transaction archive to be queried
 */
class TransactionStoreAggregator : public TransactionStoreInterface
{
public:
  TransactionStoreAggregator(TransactionStoreInterface &pool, TransactionStoreInterface &store);
  ~TransactionStoreAggregator() override = default;

  /// @name Transaction Storage Interface
  /// @{
  void     Add(chain::Transaction const &tx) override;
  bool     Has(Digest const &tx_digest) const override;
  bool     Get(Digest const &tx_digest, chain::Transaction &tx) const override;
  uint64_t GetCount() const override;
  /// @}

private:
  TransactionStoreInterface &pool_;
  TransactionStoreInterface &store_;
};

}  // namespace ledger
}  // namespace fetch
