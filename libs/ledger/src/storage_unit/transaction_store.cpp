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

#include "chain/transaction_rpc_serializers.hpp"
#include "ledger/storage_unit/transaction_store.hpp"
#include "logging/logging.hpp"

namespace fetch {
namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "TransactionStore";

using fetch::storage::ResourceID;

using TxArray = TransactionStore::TxArray;

ResourceID CreateResourceId(Digest const &digest)
{
  return ResourceID{digest};
}

}  // namespace

void TransactionStore::New(std::string const &doc_file, std::string const &index_file, bool create)
{
  archive_.New(doc_file, index_file, create);
}

void TransactionStore::Load(std::string const &doc_file, std::string const &index_file, bool create)
{
  archive_.Load(doc_file, index_file, create);
}

/**
 * Add a transaction to the store
 *
 * @param tx The transaction to set added to storage
 */
void TransactionStore::Add(chain::Transaction const &tx)
{
  auto const rid = CreateResourceId(tx.digest());

  try
  {
    if (!archive_.Has(rid))
    {
      archive_.Set(rid, tx);
    }
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to add tx: 0x", tx.digest().ToHex(),
                   " to store: ", ex.what());
  }
}

/**
 * Check to see if requested transaction exists
 *
 * @param tx_digest The transaction digest to be searched for
 * @return true if present, otherwise false
 */
bool TransactionStore::Has(Digest const &tx_digest) const
{
  auto const rid = CreateResourceId(tx_digest);
  return archive_.Has(rid);
}

/**
 * Lookup a transaction from the store
 *
 * @param tx_digest The transaction digest to lookup
 * @param tx
 * @return
 */
bool TransactionStore::Get(Digest const &tx_digest, chain::Transaction &tx) const
{
  auto const rid = CreateResourceId(tx_digest);

  try
  {
    return archive_.Get(rid, tx);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to get tx: 0x", tx.digest().ToHex(),
                   " from store: ", ex.what());
  }

  return false;
}

/**
 * Get the total number of transactions in this store
 *
 * @return The number of transactions stored
 */
uint64_t TransactionStore::GetCount() const
{
  return static_cast<uint64_t>(archive_.size());
}

/**
 * Pull a sub tree from the storage engine with the given starting prefix for the digest
 *
 * @param partial_digest The partial digest for the subtree
 * @param bit_count The bit count of the partial digest for the subtree
 * @param pull_limit The maximum number of transactions to be retrieved
 * @return The extracted subtree of transactions from the store
 */
TxArray TransactionStore::PullSubtree(Digest const &partial_digest, uint64_t bit_count,
                                      uint64_t pull_limit)
{
  TxArray ret{};

  uint64_t counter = 0;

  archive_.Flush(false);
  archive_.WithLock([this, &pull_limit, &ret, &counter, &partial_digest, bit_count]() {
    // This is effectively saying get all objects whose ID begins rid & mask
    auto it = archive_.GetSubtree(ResourceID(partial_digest), bit_count);

    while ((it != archive_.end()) && (counter++ < pull_limit))
    {
      ret.push_back(*it);
      ++it;
    }
  });

  return ret;
}

}  // namespace ledger
}  // namespace fetch
