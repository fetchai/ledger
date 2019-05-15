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

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <unordered_set>

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
  uint64_t total_weight{0};
};

enum class BlockStatus
{
  ADDED,      ///< The block has been added to the chain
  LOOSE,      ///< The block has been added to the chain but is currently loose
  DUPLICATE,  ///< The block has been detected as a duplicate
  INVALID     ///< The block is invalid and has not been added to the chain
};

/**
 * Converts a block status into a human readable string
 *
 * @param status The status enumeration
 * @return The output text
 */
inline constexpr char const *ToString(BlockStatus status)
{
  switch (status)
  {
  case BlockStatus::ADDED:
    return "Added";
  case BlockStatus::LOOSE:
    return "Loose";
  case BlockStatus::DUPLICATE:
    return "Duplicate";
  case BlockStatus::INVALID:
    return "Invalid";
  }

  return "Unknown";
}

class MainChain
{
public:
  using BlockPtr     = std::shared_ptr<Block const>;
  using Blocks       = std::vector<BlockPtr>;
  using BlockHash    = Block::Digest;
  using BlockHashes  = std::vector<BlockHash>;
  using BlockHashSet = std::unordered_set<BlockHash>;

  static constexpr char const *LOGGING_NAME = "MainChain";
  static constexpr uint64_t    UPPER_BOUND  = 100000ull;

  enum class Mode
  {
    IN_MEMORY_DB = 0,
    CREATE_PERSISTENT_DB,
    LOAD_PERSISTENT_DB
  };

  // When traversing the chain and returning a subset due to hitting a limit,
  // either return blocks closer to genesis (least recent in time), or
  // return closer to head (most recent)
  enum class BehaviourWhenLimit
  {
    RETURN_MOST_RECENT = 0,
    RETURN_LEAST_RECENT
  };

  // Construction / Destruction
  explicit MainChain(Mode mode = Mode::IN_MEMORY_DB);
  MainChain(MainChain const &rhs) = delete;
  MainChain(MainChain &&rhs)      = delete;
  ~MainChain();

  /// @name Block Management
  /// @{
  BlockStatus AddBlock(Block const &block);
  BlockPtr    GetBlock(BlockHash hash) const;
  bool        RemoveBlock(BlockHash hash);
  /// @}

  /// @name Chain Queries
  /// @{
  BlockPtr  GetHeaviestBlock() const;
  BlockHash GetHeaviestBlockHash() const;
  Blocks    GetHeaviestChain(uint64_t limit = UPPER_BOUND) const;
  Blocks    GetChainPreceding(BlockHash at, uint64_t limit = UPPER_BOUND) const;
  bool      GetPathToCommonAncestor(
           Blocks &blocks, BlockHash tip, BlockHash node, uint64_t limit = UPPER_BOUND,
           BehaviourWhenLimit behaviour = BehaviourWhenLimit::RETURN_MOST_RECENT) const;
  /// @}

  /// @name Tips
  /// @{
  BlockHashSet GetTips() const;
  bool         ReindexTips();  // testing only
  /// @}

  /// @name Missing / Loose Management
  /// @{
  BlockHashSet GetMissingTips() const;
  BlockHashes  GetMissingBlockHashes(uint64_t limit = UPPER_BOUND) const;
  bool         HasMissingBlocks() const;
  /// @}

  template <typename T>
  bool StripAlreadySeenTx(BlockHash starting_hash, T &container) const;

  // Operators
  MainChain &operator=(MainChain const &rhs) = delete;
  MainChain &operator=(MainChain &&rhs) = delete;

private:
  struct DbRecord
  {
    Block block;
    // genesis (hopefully) cannot be next hash so is used as undefined value
    BlockHash next_hash = GENESIS_DIGEST;

    BlockHash hash() const
    {
      return block.body.hash;
    }
  };

  using IntBlockPtr   = std::shared_ptr<Block>;
  using BlockMap      = std::unordered_map<BlockHash, IntBlockPtr>;
  using References    = std::unordered_multimap<BlockHash, BlockHash>;
  using Proof         = Block::Proof;
  using TipsMap       = std::unordered_map<BlockHash, Tip>;
  using BlockHashList = std::list<BlockHash>;
  using LooseBlockMap = std::unordered_map<BlockHash, BlockHashList>;
  using BlockStore    = fetch::storage::ObjectStore<DbRecord>;
  using BlockStorePtr = std::unique_ptr<BlockStore>;
  using RMutex        = std::recursive_mutex;
  using RLock         = std::unique_lock<RMutex>;

  struct HeaviestTip
  {
    uint64_t  weight{0};
    BlockHash hash{GENESIS_DIGEST};

    bool Update(Block const &);
  };

  /// @name Persistence Management
  /// @{
  void RecoverFromFile(Mode mode);
  void WriteToFile();
  void TrimCache();
  void FlushBlock(IntBlockPtr const &block);
  /// @}

  /// @name Loose Blocks
  /// @{
  void CompleteLooseBlocks(IntBlockPtr const &block);
  void RecordLooseBlock(IntBlockPtr const &block);
  /// @}

  /// @name Block Lookup
  /// @{
  BlockStatus InsertBlock(IntBlockPtr const &block, bool evaluate_loose_blocks = true);
  bool        LookupBlock(BlockHash hash, IntBlockPtr &block, bool add_to_cache = false) const;
  bool        LookupBlockFromCache(BlockHash hash, IntBlockPtr &block) const;
  bool        LookupBlockFromStorage(BlockHash hash, IntBlockPtr &block, bool add_to_cache) const;
  bool        IsBlockInCache(BlockHash hash) const;
  void        AddBlockToCache(IntBlockPtr const &) const;
  /// @}

  /// @name Low-level storage interface
  /// @{
  void                CacheBlock(IntBlockPtr const &block) const;
  BlockMap::size_type UncacheBlock(BlockHash hash) const;
  void                KeepBlock(IntBlockPtr const &block) const;
  bool                LoadBlock(BlockHash const &hash, Block &block) const;
  /// @}

  /// @name Tip Management
  /// @{
  bool AddTip(IntBlockPtr const &block);
  bool UpdateTips(IntBlockPtr const &block);
  bool DetermineHeaviestTip();
  /// @}

  static IntBlockPtr CreateGenesisBlock();

  BlockHash GetHeadHash();
  void      SetHeadHash(BlockHash const &hash);

  bool RemoveTree(BlockHash const &hash, BlockHashSet &invalidated_blocks);

  BlockStorePtr block_store_;  /// < Long term storage and backup
  std::fstream  head_store_;

  mutable RMutex   lock_;         ///< Mutex protecting block_chain_, tips_ & heaviest_
  mutable BlockMap block_chain_;  ///< All recent blocks are kept in memory
  // The whole tree of previous-next relations among cached blocks
  mutable References references_;
  TipsMap            tips_;          ///< Keep track of the tips
  HeaviestTip        heaviest_;      ///< Heaviest block/tip
  LooseBlockMap      loose_blocks_;  ///< Waiting (loose) blocks

  /**
   * Serializer for the DbRecord
   *
   * @tparam T The serializer type
   * @param serializer The reference to hte serializer
   * @param dbRecord The reference to the DbRecord to be serialised
   */
  template <typename T>
  friend void Serialize(T &serializer, DbRecord const &dbRecord)
  {
    serializer << dbRecord.block << dbRecord.next_hash;
  }

  /**
   * Deserializer for the DbRecord
   *
   * @tparam T The serializer type
   * @param serializer The reference to the serializer
   * @param dbRecord The reference to the output dbRecord to be populated
   */
  template <typename T>
  friend void Deserialize(T &serializer, DbRecord &dbRecord)
  {
    serializer >> dbRecord.block >> dbRecord.next_hash;
  }
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
bool MainChain::StripAlreadySeenTx(BlockHash starting_hash, T &container) const
{
  using namespace std::chrono;
  using Clock = high_resolution_clock;

  FETCH_LOG_DEBUG(LOGGING_NAME, "Starting TX uniqueness verify");

  std::size_t blocks_checked = 0;
  auto const  start_time     = Clock::now();

  IntBlockPtr block;
  if (!LookupBlock(starting_hash, block, false) || block->is_loose)
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

  for (;;)
  {
    ++blocks_checked;

    // Traversing the chain fully is costly: break out early if we know the transactions are all
    // duplicated (or empty)
    if (transactions_to_check.size() == transactions_duplicated.size())
    {
      break;
    }

    for (auto const &slice : block->body.slices)
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

    // exit the loop once we can no longer find the block
    if (!LookupBlock(block->body.previous_hash, block, false))
    {
      break;
    }
  }

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

  // determine the time this function has taken to execute
  auto const delta_time_ms = duration_cast<milliseconds>(Clock::now() - start_time).count();

  if (delta_time_ms >= 100)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Finished TX uniqueness verify in: ", delta_time_ms, "ms",
                   " checked blocks: ", blocks_checked);
  }
  else
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Finished TX uniqueness verify in: ", delta_time_ms, "ms",
                    " checked blocks: ", blocks_checked);
  }

  return true;
}

}  // namespace ledger
}  // namespace fetch
