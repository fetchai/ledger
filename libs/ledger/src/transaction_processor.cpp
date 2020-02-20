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
#include "core/set_thread_name.hpp"
#include "ledger/block_packer_interface.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "ledger/transaction_processor.hpp"
#include "ledger/transaction_status_cache.hpp"

#include <cstddef>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

using fetch::chain::Transaction;
using fetch::chain::TransactionLayout;

namespace fetch {
namespace ledger {

namespace {
constexpr char const *LOGGING_NAME = "TransactionProcessor";
}

/**
 * Transaction Processor constructor
 *
 * @param storage The reference to the storage unit
 * @param miner The reference to the system miner
 */
TransactionProcessor::TransactionProcessor(DAGPtr dag, StorageUnitInterface &storage,
                                           BlockPackerInterface &packer,
                                           TxStatusCachePtr      tx_status_cache,
                                           std::size_t           num_threads)
  : dag_{std::move(dag)}
  , storage_{storage}
  , packer_{packer}
  , status_cache_{std::move(tx_status_cache)}
  , verifier_{*this, num_threads, "TxV-P"}
  , running_{false}
{}

TransactionProcessor::~TransactionProcessor()
{
  Stop();
}

void TransactionProcessor::OnTransaction(TransactionPtr const &tx)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Verified Input Transaction: 0x", tx->digest().ToHex());

  // dispatch the transaction to the storage engine
  try
  {
    storage_.AddTransaction(*tx);
  }
  catch (std::exception const &e)
  {
    // TODO(unknown): We need to think about how we handle failures of that class.
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to add transaction to storage: ", e.what());
    return;
  }

  switch (tx->contract_mode())
  {
  case Transaction::ContractMode::NOT_PRESENT:
  case Transaction::ContractMode::PRESENT:
  case Transaction::ContractMode::CHAIN_CODE:

    // dispatch the summary to the miner
    packer_.EnqueueTransaction(*tx);

    // update the status cache with the state of this transaction
    if (status_cache_)
    {
      status_cache_->Update(tx->digest(), TransactionStatus::PENDING);
    }
    break;

  case Transaction::ContractMode::SYNERGETIC:

    if (tx->action() == "data" && dag_)
    {
      dag_->AddTransaction(*tx, DAGInterface::DAGTypes::DATA);

      // update the status cache with the state of this transaction
      if (status_cache_)
      {
        status_cache_->Update(tx->digest(), TransactionStatus::SUBMITTED);
      }
    }

    break;
  }
}

void TransactionProcessor::ThreadEntryPoint()
{
  SetThreadName("TxProc");

  std::vector<TransactionLayout> new_txs;
  while (running_)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    new_txs.clear();
    new_txs = storage_.PollRecentTx(10000);

    if (!new_txs.empty())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Pulled ", new_txs.size(), " transactions from shards");
    }

    for (auto const &summary : new_txs)
    {
      packer_.EnqueueTransaction(summary);
    }
  }
}

/**
 * Start the transaction processor
 */
void TransactionProcessor::Start()
{
  verifier_.Start();
  running_ = true;
  poll_new_tx_thread_ =
      std::make_unique<std::thread>(&TransactionProcessor::ThreadEntryPoint, this);
}

/**
 * Stop the transactions processor
 */
void TransactionProcessor::Stop()
{
  running_ = false;
  if (poll_new_tx_thread_)
  {
    poll_new_tx_thread_->join();
    poll_new_tx_thread_.reset();
  }

  verifier_.Stop();
}

/**
 * Add a single transaction to the processor
 *
 * @param tx The reference to the new transaction to be processed
 */
void TransactionProcessor::AddTransaction(TransactionPtr const &tx)
{
  verifier_.AddTransaction(tx);
}

/**
 * Add a single transaction to the processor
 *
 * @param tx The reference to the new transaction to be processed
 */
void TransactionProcessor::AddTransaction(TransactionPtr &&tx)
{
  verifier_.AddTransaction(std::move(tx));
}

}  // namespace ledger
}  // namespace fetch
