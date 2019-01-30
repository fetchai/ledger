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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/mutex.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include "ledger/chain/constants.hpp"
#include "ledger/chain/transaction.hpp"
#include "network/generics/milli_timer.hpp"
#include "storage/object_store.hpp"
#include "storage/resource_mapper.hpp"

#include <map>
#include <memory>
#include <set>

namespace fetch {
namespace ledger {

/**
 * Main chain contains and manages block headers. No verification of headers is
 * done, the structure will purely accept new headers and provide the current
 * heaviest chain. Tie breaks decided on a hash comparison.
 * The only high cost operation O(n) should be when adding
 * blocks that complete a lot of loose blocks.
 *
 * Note:
 *     All blocks added MUST have a valid hash and previous hash
 *
 * Loose blocks are blocks where the previous hash/block isn't found.
 * Tips keep track of all non loose chains.
 */
struct Tip
{
  uint64_t total_weight;
};

class MainChain
{
public:
  using Blocks     = std::vector<Block>;
  using BlockHash  = Block::Digest;
  using BlockHashs = std::vector<BlockHash>;

  // Hard code genesis on construction

  // Construction / Destruction
  explicit MainChain(bool disable_persistence = false);
  MainChain(MainChain const &rhs) = delete;
  MainChain(MainChain &&rhs)      = delete;
  ~MainChain()                    = default;

  bool AddBlock(Block &block, bool recursive_iteration = false);

  Block const &HeaviestBlock() const;

  uint64_t weight() const
  {
    return heaviest_.first;
  }

  std::size_t totalBlocks() const
  {
    return block_chain_.size();
  }

  Blocks HeaviestChain(uint64_t const &limit = std::numeric_limits<uint64_t>::max()) const;

  Blocks ChainPreceding(BlockHash const &at,
                        uint64_t         limit = std::numeric_limits<uint64_t>::max()) const;

  bool GetCommonSubTree(Blocks &blocks, BlockHash tip, BlockHash node,
                        uint64_t limit = std::numeric_limits<uint64_t>::max());

  void Reset();

  // TODO(private issue 512): storage code blocks this being const
  bool Get(BlockHash hash, Block &block);

  BlockHashs GetMissingBlockHashes(std::size_t maximum);

  bool HasMissingBlocks() const;

  template <typename T>
  bool StripAlreadySeenTx(BlockHash starting_hash, T &container);

  // Operators
  MainChain &operator=(MainChain const &rhs) = delete;
  MainChain &operator=(MainChain &&rhs) = delete;

private:
  using BlockMap           = std::unordered_map<BlockHash, Block>;
  using Proof              = Block::Proof;
  using TipPtr             = std::shared_ptr<Tip>;
  using TipsMap            = std::unordered_map<BlockHash, TipPtr>;
  using HeaviestTip        = std::pair<uint64_t, BlockHash>;
  using LooseBlockMap      = std::unordered_map<BlockHash, BlockHashs>;
  using BlockStore         = fetch::storage::ObjectStore<Block>;
  using BlockStorePtr      = std::unique_ptr<BlockStore>;
  using TransactionSummary = chain::TransactionSummary;
  using RMutex             = std::recursive_mutex;
  using RLock              = std::unique_lock<RMutex>;

  static constexpr char const *LOGGING_NAME        = "MainChain";
  static constexpr uint32_t    block_confirmation_ = 10;

  void RecoverFromFile();
  void WriteToFile();
  void FlushBlock(Block const &block);
  bool GetPrev(Block &block);
  bool GetPrevFromStore(Block &block);
  void CompleteLooseBlocks(Block const &block);
  void NewLooseBlock(Block &block);
  bool CheckDiskForBlock(Block &block);
  bool UpdateTips(Block &block, Block const &prev_block);

  static Block CreateGenesisBlock();

  BlockStorePtr block_store_;  /// < Long term storage and backup

  mutable RMutex main_mutex_;
  BlockMap       block_chain_;  ///< all recent blocks are here
  TipsMap        tips_;         ///< Keep track of the tips
  HeaviestTip    heaviest_;     ///< Heaviest block/tip

  mutable RMutex loose_mutex_;
  LooseBlockMap  loose_blocks_;  ///< Waiting (loose) blocks
};

/**
 * Strip transactions in container that already exist in the blockchain
 *
 * @param: starting_hash Block to start looking downwards from
 * @tparam: container Container to remove transactions from
 *
 * @return: bool whether the starting hash referred to a valid block on a valid chain
 */
template <typename T>
bool MainChain::StripAlreadySeenTx(BlockHash starting_hash, T &container)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Starting TX uniqueness verify");

  Block       block;
  std::size_t blocks_checked = 0;
  auto        t1             = std::chrono::high_resolution_clock::now();

  if (!Get(starting_hash, block) || block.is_loose)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "TX uniqueness verify on bad block hash");
    return false;
  }

  // Need a set for quickly checking whether transactions are in our container
  std::set<TransactionSummary>    transactions_to_check;
  std::vector<TransactionSummary> transactions_duplicated;

  for (auto const &tx : container)
  {
    transactions_to_check.insert(tx.transaction);
  }

  do
  {
    ++blocks_checked;

    for (auto const &slice : block.body.slices)
    {
      for (auto const &tx : slice)
      {
        auto it = transactions_to_check.find(tx);

        // Found a TX in the blockchain that is in our container
        if (it != transactions_to_check.end())
        {
          transactions_duplicated.push_back(tx);
        }
      }
    }
  } while (Get(block.body.previous_hash,
               block));  // Note we don't need to hold the lock for the whole search

  std::size_t duplicated_counter = 0;

  // remove duplicate transactions
  if (!transactions_duplicated.empty())
  {
    FETCH_LOG_INFO(LOGGING_NAME,
                   "TX uniqueness verify - found duplicate TXs!: ", transactions_duplicated.size());

    // Iterate our container
    auto it = container.cbegin();

    // Remove this item from our container if is duplicate
    while (it != container.end())
    {
      // We expect the number of duplicates to be low so vector search should be fine
      if (std::find(transactions_duplicated.begin(), transactions_duplicated.end(),
                    (*it).transaction) != transactions_duplicated.end())
      {
        it = container.erase(it);
        duplicated_counter++;
        continue;
      }

      ++it;
    }
  }

  if (duplicated_counter != transactions_duplicated.size())
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Warning! Duplicated transactions might not be removed from block. Seen: ",
                   transactions_duplicated.size(), " Removed: ", duplicated_counter);
  }

  auto   t2         = std::chrono::high_resolution_clock::now();
  double time_taken = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
  FETCH_LOG_INFO(LOGGING_NAME, "Finished TX uniqueness verify - time (s): ", time_taken,
                 " checked blocks: ", blocks_checked);
  return true;
}

}  // namespace ledger
}  // namespace fetch
