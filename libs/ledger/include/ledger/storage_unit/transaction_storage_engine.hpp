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

#include "chain/transaction_layout.hpp"
#include "ledger/storage_unit/recent_transaction_cache.hpp"
#include "ledger/storage_unit/transaction_archiver.hpp"
#include "ledger/storage_unit/transaction_memory_pool.hpp"
#include "ledger/storage_unit/transaction_storage_engine_interface.hpp"
#include "ledger/storage_unit/transaction_store.hpp"
#include "ledger/storage_unit/transaction_store_aggregator.hpp"

#include <functional>

namespace fetch {
namespace ledger {

class TransactionStorageEngine : public TransactionStorageEngineInterface
{
public:
  // Construction / Destruction
  explicit TransactionStorageEngine(uint32_t log2_num_lanes);
  TransactionStorageEngine(TransactionStorageEngine const &) = delete;
  TransactionStorageEngine(TransactionStorageEngine &&)      = delete;
  ~TransactionStorageEngine() override                       = default;

  void New(std::string const &doc_file, std::string const &index_file, bool const &create = true);
  void Load(std::string const &doc_file, std::string const &index_file, bool const &create = true);

  /// @name Transaction Storage Engine Interface
  /// @{
  void        Add(chain::Transaction const &tx, bool is_recent) override;
  bool        Has(Digest const &tx_digest) const override;
  bool        Get(Digest const &tx_digest, chain::Transaction &tx) const override;
  std::size_t GetCount() const override;
  void        Confirm(Digest const &tx_digest) override;
  TxLayouts   GetRecent(uint32_t max_to_poll) override;
  TxArray     PullSubtree(Digest const &partial_digest, uint64_t bit_count,
                          uint64_t pull_limit) override;
  /// @}

  //  void SetCallback(Callback cb)
  //  {
  //    set_callback_ = std::move(cb);
  //  }

  // Operators
  TransactionStorageEngine &operator=(TransactionStorageEngine const &) = delete;
  TransactionStorageEngine &operator=(TransactionStorageEngine &&) = delete;

private:
  static const std::size_t MAX_NUM_RECENT_TX = 1u << 15u;

  TransactionMemoryPool      mem_pool_;
  TransactionStore           archive_;
  TransactionStoreAggregator store_{mem_pool_, archive_};
  TransactionArchiver        archiver_{mem_pool_, archive_};
  RecentTransactionsCache    recent_tx_;
};

}  // namespace ledger
}  // namespace fetch
