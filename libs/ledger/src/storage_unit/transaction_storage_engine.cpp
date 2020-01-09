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

#include "core/reactor.hpp"
#include "ledger/storage_unit/transaction_storage_engine.hpp"

namespace fetch {
namespace ledger {

using TxArray   = TransactionStorageEngineInterface::TxArray;
using TxLayouts = TransactionStorageEngineInterface::TxLayouts;

/**
 * Create a transaction storage engine with the define number of lanes
 *
 * @param log2_num_lanes The log2 of the number of lanes in the system
 * @param lane The lane id for the storage engine
 */
TransactionStorageEngine::TransactionStorageEngine(uint32_t log2_num_lanes, uint32_t lane)
  : lane_{lane}
  , recent_tx_{MAX_NUM_RECENT_TX, log2_num_lanes}
{}

/**
 * Create a new transaction archive
 *
 * @param doc_file The filename for the document file
 * @param index_file The filename for the index file
 * @param create Flag to signal if the file should be created if it doesn't exist
 */
void TransactionStorageEngine::New(std::string const &doc_file, std::string const &index_file,
                                   bool const &create)
{
  archive_.New(doc_file, index_file, create);
}

/**
 * Load an existing transaction archive
 *
 * @param doc_file The filename for the document file
 * @param index_file The filename for the index file
 * @param create Flag to signal if the file should be created if it doesn't exist
 */
void TransactionStorageEngine::Load(std::string const &doc_file, std::string const &index_file,
                                    bool const &create)
{
  archive_.Load(doc_file, index_file, create);
}

/**
 * Attach state machines to the reactor
 *
 * @param reactor The reactor to attach to
 */
void TransactionStorageEngine::AttachToReactor(core::Reactor &reactor)
{
  reactor.Attach(archiver_.GetStateMachine());
}

/**
 * Set the new transaction handler
 *
 * @note Not thread safe, should only be called on lane service setup
 *
 * @param cb The callback to be set
 */
void TransactionStorageEngine::SetNewTransactionHandler(Callback cb)
{
  new_tx_callback_ = std::move(cb);
}

/**
 * Add a new transaction to the storage engine
 *
 * @param tx The transaction to be stored
 * @param is_recent flag to signal if it is a recently seen transaction
 */
void TransactionStorageEngine::Add(chain::Transaction const &tx, bool is_recent)
{
  // add the transaction to the store
  store_.Add(tx);

  // add to the recent cache if that is the case
  if (is_recent)
  {
    recent_tx_.Add(tx);
  }

  // if there is a new tx callback then dispatch it
  if (new_tx_callback_)
  {
    new_tx_callback_(tx);
  }
}

/**
 * Query if a transaction is present in the storage engine
 *
 * @param tx_digest The digest of the transaction being queried
 * @return true if the transaction is present, otherwise false
 */
bool TransactionStorageEngine::Has(Digest const &tx_digest) const
{
  return store_.Has(tx_digest);
}

/**
 * Retrieve a transaction from the storage engine
 *
 * @param tx_digest The digest of the transaction being queried
 * @param tx The output transaction to be populated
 * @return true if successful, otherwise false
 */
bool TransactionStorageEngine::Get(Digest const &tx_digest, chain::Transaction &tx) const
{
  return store_.Get(tx_digest, tx);
}

/**
 * Get the total number of stored transactions in this storage engine
 *
 * @return The total number of transactions
 */
std::size_t TransactionStorageEngine::GetCount() const
{
  return store_.GetCount();
}

/**
 * Confirm that a transaction should be kept (has been included in the block chain)
 *
 * @param tx_digest The digest of the transaction being confirmed
 */
void TransactionStorageEngine::Confirm(Digest const &tx_digest)
{
  archiver_.Confirm(tx_digest);
}

/**
 * Get a set of transaction layouts corresponding to the most recent transactions received
 *
 * @param max_to_poll The maximum number of transactions be returned
 * @return The set of transaction layouts for the most recent transactions seen
 */
TxLayouts TransactionStorageEngine::GetRecent(uint32_t max_to_poll)
{
  return recent_tx_.Flush(max_to_poll);
}

/**
 * Pull a sub tree from the storage engine with the given starting prefix for the digest
 *
 * @param partial_digest The partial digest for the subtree
 * @param bit_count The bit count of the partial digest for the subtree
 * @param pull_limit The maximum number of transactions to be retrieved
 * @return The extracted subtree of transactions from the store
 */
TxArray TransactionStorageEngine::PullSubtree(Digest const &partial_digest, uint64_t bit_count,
                                              uint64_t pull_limit)
{
  return archive_.PullSubtree(partial_digest, bit_count, pull_limit);
}

}  // namespace ledger
}  // namespace fetch
