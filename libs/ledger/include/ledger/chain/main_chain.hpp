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

#include "bloom_filter/progressive_bloom_filter.hpp"
#include "chain/constants.hpp"
#include "chain/transaction_layout.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/digest.hpp"
#include "core/mutex.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chain/block.hpp"
#include "meta/type_util.hpp"
#include "network/generics/milli_timer.hpp"
#include "storage/object_store.hpp"
#include "storage/resource_mapper.hpp"
#include "telemetry/telemetry.hpp"

#include <cstdint>
#include <fstream>
#include <list>
#include <memory>
#include <mutex>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
  using BlockHash = Block::Hash;
  using Weight    = Block::Weight;

  BlockHash hash{fetch::chain::ZERO_HASH};
  Weight    total_weight{0};
  Weight    weight{0};
  uint64_t  block_number{0};

  constexpr Tip() = default;
  explicit Tip(Block const &block);

  Tip &operator=(Block const &block);

  bool operator<(Tip const &right) const;
  bool operator<(Block const &right) const;

  bool operator==(Tip const &right) const;
  bool operator==(Block const &right) const;

protected:
  using TipStats = std::tuple<Weight, uint64_t, Weight, BlockHash const &>;
  constexpr TipStats Stats() const
  {
    return {total_weight, block_number, weight, hash};
  }
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
constexpr char const *ToString(BlockStatus status)
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

struct BlockDbRecord;

template <class B>
struct TimeTravelogue;

class MainChain
{
public:
  using BlockPtr             = std::shared_ptr<Block const>;
  using Blocks               = std::vector<BlockPtr>;
  using BlockHash            = Block::Hash;
  using BlockHashes          = std::vector<BlockHash>;
  using BlockHashSet         = std::unordered_set<BlockHash>;
  using TransactionLayoutSet = std::unordered_set<chain::TransactionLayout>;
  using Travelogue           = TimeTravelogue<BlockPtr>;

  static constexpr char const *LOGGING_NAME = "MainChain";
  static constexpr uint64_t    UPPER_BOUND  = 5000ull;

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

  void Reset();

  /// @name Block Management
  /// @{
  BlockStatus AddBlock(Block const &blk);
  BlockPtr    GetBlock(BlockHash const &hash) const;
  bool        RemoveBlock(BlockHash const &hash);
  /// @}

  /// @name Chain Queries
  /// @{
  BlockPtr   GetHeaviestBlock() const;
  BlockHash  GetHeaviestBlockHash() const;
  Blocks     GetHeaviestChain(uint64_t lowest_block_number = 0) const;
  Blocks     GetChainPreceding(BlockHash start, uint64_t lowest_block_number = 0) const;
  Travelogue TimeTravel(BlockHash current_hash) const;
  bool       GetPathToCommonAncestor(
            Blocks &blocks, BlockHash tip_hash, BlockHash node_hash, uint64_t limit = UPPER_BOUND,
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

  /// @name Transaction Duplication Filtering
  /// @{
  DigestSet DetectDuplicateTransactions(BlockHash const &           starting_hash,
                                        TransactionLayoutSet const &transactions) const;
  /// @}

  // Operators
  MainChain &operator=(MainChain const &rhs) = delete;
  MainChain &operator=(MainChain &&rhs) = delete;

  using DbRecord      = BlockDbRecord;
  using IntBlockPtr   = std::shared_ptr<Block>;
  using BlockMap      = std::unordered_map<BlockHash, IntBlockPtr>;
  using References    = std::unordered_multimap<BlockHash, BlockHash>;
  using TipsMap       = std::unordered_map<BlockHash, Tip>;
  using BlockHashList = std::list<BlockHash>;
  using LooseBlockMap = std::unordered_map<BlockHash, BlockHashList>;
  using BlockStore    = fetch::storage::ObjectStore<DbRecord>;
  using BlockStorePtr = std::unique_ptr<BlockStore>;
  using RMutex        = std::recursive_mutex;
  using RLock         = std::unique_lock<RMutex>;

  class HeaviestTip : Tip
  {
    // When a new heaviest tip is added to the chain,
    // that is not a direct successor the previous one,
    // this number is incremented
    // (if the new heaviest is the direct successor the the old one, the former simply receives
    // this current label and that's it).
    // When the heaviest subchain is discovered as a sequence of blocks starting at
    // the very first block with non-unique forward reference and ending at
    // the heaviest tip, all the blocks that fall into this chain receive this number.
    // So, if you see a block with chain_label equal to the chain_label_ of the current HeaviestTip
    // then you know that one of its forward references also leads to a block with such label,
    // and so on, up until the HeaviestTip.
    uint64_t chain_label_{0};

  public:
    using Tip::Tip;
    using Tip::operator=;

    using Tip::operator<;
    using Tip::operator==;

    // Getters
    BlockHash const &Hash() const;
    uint64_t         BlockNumber() const;
    uint64_t         ChainLabel() const;

    // Setters
    void Set(Block &block);
    bool Update(Block &block);
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
  bool LookupBlock(BlockHash const &hash, IntBlockPtr &block, BlockHash *next_hash = nullptr) const;
  IntBlockPtr LookupBlock(BlockHash const &hash) const;
  bool        LookupBlockFromCache(BlockHash const &hash, IntBlockPtr &block) const;
  bool        LookupBlockFromStorage(BlockHash const &hash, IntBlockPtr &block,
                                     BlockHash *next_hash = nullptr) const;
  bool        IsBlockInCache(BlockHash const &hash) const;
  void        AddBlockToCache(IntBlockPtr const &block) const;
  void        AddBlockToBloomFilter(Block const &block) const;
  void CacheReference(BlockHash const &hash, BlockHash const &next_hash, bool unique = false) const;
  void ForgetReference(BlockHash const &hash, BlockHash const &next_hash = {}) const;
  bool LookupReference(BlockHash const &hash, BlockHash &next_hash) const;
  /// @}t

  /// @name Low-level storage interface
  /// @{
  void                CacheBlock(IntBlockPtr const &block) const;
  BlockMap::size_type UncacheBlock(BlockHash const &hash) const;
  void                KeepBlock(IntBlockPtr const &block) const;
  bool LoadBlock(BlockHash const &hash, Block &block, BlockHash *next_hash = nullptr) const;
  /// @}

  /// @name Tip Management
  /// @{
  bool        AddTip(IntBlockPtr const &block);
  bool        UpdateTips(IntBlockPtr const &block);
  bool        DetermineHeaviestTip();
  bool        UpdateHeaviestTip(IntBlockPtr const &block);
  IntBlockPtr HeaviestChainBlockAbove(uint64_t limit) const;
  IntBlockPtr GetLabeledSubchainStart() const;
  /// @}

  static IntBlockPtr CreateGenesisBlock();

  BlockHash GetHeadHash();
  void      SetHeadHash(BlockHash const &hash);

  bool RemoveTree(BlockHash const &removed_hash, BlockHashSet &invalidated_blocks);

  void FlushToDisk();

  Mode          mode_{Mode::IN_MEMORY_DB};
  BlockStorePtr block_store_;  ///< Long term storage and backup
  std::fstream  head_store_;

  mutable RMutex   lock_;         ///< Mutex protecting block_chain_, tips_ & heaviest_
  mutable BlockMap block_chain_;  ///< All recent blocks are kept in memory

  // The whole tree of previous-next relations among cached blocks
  mutable References forward_references_;
  TipsMap            tips_;          ///< Keep track of the tips
  HeaviestTip        heaviest_;      ///< Heaviest block/tip
  LooseBlockMap      loose_blocks_;  ///< Waiting (loose) blocks
  ///< The earliest block known of current heaveiest chain.
  mutable IntBlockPtr labeled_subchain_start_;

  mutable ProgressiveBloomFilter   bloom_filter_;
  telemetry::GaugePtr<std::size_t> bloom_filter_queried_bit_count_;
  telemetry::CounterPtr            bloom_filter_query_count_;
  telemetry::CounterPtr            bloom_filter_positive_count_;
  telemetry::CounterPtr            bloom_filter_false_positive_count_;
};

}  // namespace ledger
}  // namespace fetch
