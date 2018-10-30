#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "ledger/chain/transaction.hpp"
#include "metrics/metrics.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "miner/miner_interface.hpp"

namespace fetch {
namespace ledger {

class TransactionProcessor
{
public:
  using TransactionList = std::vector<chain::Transaction>;

  TransactionProcessor(StorageUnitInterface &storage, miner::MinerInterface &miner)
    : storage_{storage}
    , miner_{miner}
  {}

  void AddTransaction(chain::Transaction const &tx)
  {
    FETCH_METRIC_TX_SUBMITTED(tx.digest());

    // tell the node about the transaction
    storage_.AddTransaction(tx);

    FETCH_METRIC_TX_STORED(tx.digest());

    // tell the miner about the transaction
    miner_.EnqueueTransaction(tx.summary());

    FETCH_METRIC_TX_QUEUED(tx.digest());
  }

  void AddTransactions(TransactionList const &txs)
  {
    using fetch::metrics::Metrics;

#ifdef FETCH_ENABLE_METRICS
    auto const submitted = Metrics::Clock::now();
#endif // FETCH_ENABLE_METRICS

    storage_.AddTransactions(txs);

#ifdef FETCH_ENABLE_METRICS
    auto const stored = Metrics::Clock::now();
#endif // FETCH_ENABLE_METRICS

    for (auto const &tx : txs)
    {
      miner_.EnqueueTransaction(tx.summary());
    }

#ifdef FETCH_ENABLE_METRICS
    auto const queued = Metrics::Clock::now();
#endif // FETCH_ENABLE_METRICS

    // dispatch the metrics
#ifdef FETCH_ENABLE_METRICS
    for (auto const &tx : txs)
    {
      FETCH_METRIC_TX_SUBMITTED_EX(tx.digest(), submitted);
      FETCH_METRIC_TX_STORED_EX(tx.digest(), stored);
      FETCH_METRIC_TX_QUEUED_EX(tx.digest(), queued);
    }
#endif // FETCH_ENABLE_METRICS
  }

private:
  StorageUnitInterface & storage_;
  miner::MinerInterface &miner_;
};

}  // namespace ledger
}  // namespace fetch
