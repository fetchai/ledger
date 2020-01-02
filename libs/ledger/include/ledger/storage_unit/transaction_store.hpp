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
#include "ledger/storage_unit/transaction_store_interface.hpp"
#include "storage/object_store.hpp"

#include <string>
#include <vector>

namespace fetch {
namespace ledger {

class TransactionStore : public TransactionStoreInterface
{
public:
  using TxArray = std::vector<chain::Transaction>;

  // Construction / Destruction
  TransactionStore()                         = default;
  TransactionStore(TransactionStore const &) = delete;
  TransactionStore(TransactionStore &&)      = delete;
  ~TransactionStore() override               = default;

  // Database control
  void New(std::string const &doc_file, std::string const &index_file, bool create = true);
  void Load(std::string const &doc_file, std::string const &index_file, bool create = true);

  /// @name Transaction Storage Interface
  /// @{
  void     Add(chain::Transaction const &tx) override;
  bool     Has(Digest const &tx_digest) const override;
  bool     Get(Digest const &tx_digest, chain::Transaction &tx) const override;
  uint64_t GetCount() const override;
  /// @}

  /// @mame Low Level Subtree Access
  /// @{
  TxArray PullSubtree(Digest const &partial_digest, uint64_t bit_count, uint64_t pull_limit);
  /// @}

  // Operators
  TransactionStore &operator=(TransactionStore const &) = delete;
  TransactionStore &operator=(TransactionStore &&) = delete;

private:
  using Archive = storage::ObjectStore<chain::Transaction>;

  mutable Archive archive_;
};

}  // namespace ledger
}  // namespace fetch
