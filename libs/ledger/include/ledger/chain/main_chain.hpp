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

#include "bloom_filter/bloom_filter.hpp"
#include "chain/constants.hpp"
#include "chain/transaction_layout.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/digest.hpp"
#include "core/mutex.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include "network/generics/milli_timer.hpp"
#include "storage/object_store.hpp"
#include "storage/resource_mapper.hpp"
#include "telemetry/telemetry.hpp"

#include <cstdint>
#include <fstream>
#include <list>
#include <memory>
#include <mutex>
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
  uint64_t total_weight{0};
  uint64_t weight{0};
  uint64_t block_number{0};
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
constexpr char const *ToString(BlockStatus status);

struct BlockDbRecord;

class MainChain
{
public:
  using BlockPtr             = std::shared_ptr<Block const>;
  using Blocks               = std::vector<BlockPtr>;
  using BlockHash            = Block::Hash;
  using BlockHashes          = std::vector<BlockHash>;
  using BlockHashSet         = std::unordered_set<BlockHash>;
  using TransactionLayoutSet = std::unordered_set<chain::TransactionLayout>;

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
  explicit MainChain(bool enable_bloom_filter = false, Mode mode = Mode::IN_MEMORY_DB);
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
  BlockPtr  GetHeaviestBlock() const;
  BlockHash GetHeaviestBlockHash() const;
  Blocks    GetHeaviestChain(uint64_t limit = UPPER_BOUND) const;
  Blocks    GetChainPreceding(BlockHash start, uint64_t limit = UPPER_BOUND) const;
  Blocks    TimeTravel(BlockHash start, int64_t limit = static_cast<int64_t>(UPPER_BOUND)) const;
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

  /// @name Transaction Duplication Filtering
  /// @{
  DigestSet DetectDuplicateTransactions(BlockHash const &starting_hash,
                                        DigestSet const &transactions) const;
  /// @}

  // Operators
  MainChain &operator=(MainChain const &rhs) = delete;
  MainChain &operator=(MainChain &&rhs) = delete;

  using DbRecord      = BlockDbRecord;
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
    uint64_t total_weight{0};
    uint64_t weight{0};
    uint64_t block_number{0};
    // assuming every chain has a proper genesis
    BlockHash hash{chain::GENESIS_DIGEST};

    bool Update(Block const &block);
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
  bool LookupBlock(BlockHash const &hash, IntBlockPtr &block, bool add_to_cache = false) const;
  bool LookupBlockFromCache(BlockHash const &hash, IntBlockPtr &block) const;
  bool LookupBlockFromStorage(BlockHash const &hash, IntBlockPtr &block, bool add_to_cache) const;
  bool IsBlockInCache(BlockHash const &hash) const;
  void AddBlockToCache(IntBlockPtr const &block) const;
  void AddBlockToBloomFilter(Block const &block) const;
  /// @}

  /// @name Low-level storage interface
  /// @{
  void                CacheBlock(IntBlockPtr const &block) const;
  BlockMap::size_type UncacheBlock(BlockHash const &hash) const;
  void                KeepBlock(IntBlockPtr const &block) const;
  bool LoadBlock(BlockHash const &hash, Block &block, BlockHash *next_hash = nullptr) const;
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

  bool RemoveTree(BlockHash const &removed_hash, BlockHashSet &invalidated_blocks);

  BlockStorePtr block_store_;  /// < Long term storage and backup
  std::fstream  head_store_;

  mutable RMutex   lock_;         ///< Mutex protecting block_chain_, tips_ & heaviest_
  mutable BlockMap block_chain_;  ///< All recent blocks are kept in memory
  // The whole tree of previous-next relations among cached blocks
  mutable References                references_;
  TipsMap                           tips_;          ///< Keep track of the tips
  HeaviestTip                       heaviest_;      ///< Heaviest block/tip
  LooseBlockMap                     loose_blocks_;  ///< Waiting (loose) blocks
  std::unique_ptr<BasicBloomFilter> bloom_filter_;
  bool const                        enable_bloom_filter_;
  telemetry::GaugePtr<std::size_t>  bloom_filter_queried_bit_count_;
  telemetry::CounterPtr             bloom_filter_query_count_;
  telemetry::CounterPtr             bloom_filter_positive_count_;
  telemetry::CounterPtr             bloom_filter_false_positive_count_;
};

}  // namespace ledger
}  // namespace fetch
