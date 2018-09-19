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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/mutex.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include "ledger/chain/transaction.hpp"
#include "network/generics/milli_timer.hpp"
#include "storage/object_store.hpp"
#include "storage/resource_mapper.hpp"
#include <map>
#include <memory>
#include <set>

// Specialise hash of byte_array for unordered map/set
namespace std {

template <>
struct hash<fetch::byte_array::ByteArray>
{
  std::size_t operator()(const fetch::byte_array::ByteArray &k) const
  {
    assert(k.size() >= 8);
    return *reinterpret_cast<std::size_t const *>(k.pointer());
  }
};
}  // namespace std

namespace fetch {
namespace chain {

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
 **/
struct Tip
{
  uint64_t total_weight;
};

class MainChain
{
public:
  using BlockType = BasicBlock<fetch::chain::consensus::ProofOfWork, fetch::crypto::SHA256>;
  using BlockHash = fetch::byte_array::ByteArray;
  using PrevHash  = fetch::byte_array::ByteArray;
  using ProofType = BlockType::proof_type;

  static constexpr char const *LOGGING_NAME = "MainChain";

  // Hard code genesis on construction
  // TODO(EJF): Rename minerNumber_
  MainChain(uint32_t miner_number = std::numeric_limits<uint32_t>::max())
    : minerNumber_{miner_number}
  {
    BlockType genesis;
    genesis.UpdateDigest();
    // TODO(EJF): Ensure that this is correct
    // genesis.body().previous_hash = genesis.hash();

    // Add genesis to block chain
    genesis.loose()             = false;
    blockChain_[genesis.hash()] = genesis;

    // Create tip for genesis
    auto tip              = std::make_shared<Tip>();
    tip->total_weight     = genesis.weight();
    tips_[genesis.hash()] = tip;
    heaviest_             = std::make_pair(genesis.weight(), genesis.hash());

    if (minerNumber_ != std::numeric_limits<uint32_t>::max())
    {
      RecoverFromFile();
      saving_to_file_ = true;
    }
  }

  MainChain(MainChain const &rhs) = delete;
  MainChain(MainChain &&rhs)      = delete;
  MainChain &operator=(MainChain const &rhs) = delete;
  MainChain &operator=(MainChain &&rhs) = delete;

  bool AddBlock(BlockType &block, bool recursive_iteration = false)
  {
    fetch::generics::MilliTimer           myTimer("MainChain::AddBlock");
    std::unique_lock<fetch::mutex::Mutex> lock(main_mutex_);

    if (block.hash().size() == 0)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Erk! You called AddBlock with no UpdateDigest");
    }

    BlockType prev_block;

    if (!recursive_iteration)
    {
      // First check if block already exists (not checking in object store)
      if (blockChain_.find(block.hash()) != blockChain_.end())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Attempting to add already seen block");
        return false;
      }

      // Look for the prev block to this one
      auto it_prev = blockChain_.find(block.body().previous_hash);

      if (it_prev == blockChain_.end())
      {
        // Note: think about rolling back into FS
        FETCH_LOG_INFO(LOGGING_NAME, "Block prev not found");
        // We can't find the prev, this is probably a loose block.
        lock.unlock();
        return CheckDiskForBlock(block);
      }

      prev_block = (*it_prev).second;

      // Also a loose block if it points to a loose block
      if (prev_block.loose() == true)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Block connects to loose block");
        NewLooseBlock(block);
        return true;
      }
    }
    else
    {
      prev_block = blockChain_[block.body().previous_hash];
    }

    // At this point we have a new block with a prev that's known and not loose. Update tips
    block.loose()         = false;
    bool heaviestAdvanced = UpdateTips(block, prev_block);

    // Add block
    FETCH_LOG_DEBUG(LOGGING_NAME, "Adding block to chain: ", ToBase64(block.hash()));
    blockChain_[block.hash()] = block;

    if (heaviestAdvanced)
    {
      WriteToFile();
    }

    // Now we're done, it's possible this added block completed some loose blocks.
    main_mutex_.unlock();

    if (!recursive_iteration)
    {
      CompleteLooseBlocks(block);
    }

    return true;
  }

  BlockType const &HeaviestBlock() const
  {
    std::lock_guard<fetch::mutex::Mutex> lock(main_mutex_);

    auto const &block = blockChain_.at(heaviest_.second);

    return block;
  }

  uint64_t weight() const
  {
    return heaviest_.first;
  }

  std::size_t totalBlocks() const
  {
    return blockChain_.size();
  }

  std::vector<BlockType> HeaviestChain(
      uint64_t const &limit = std::numeric_limits<uint64_t>::max()) const
  {
    fetch::generics::MilliTimer          myTimer("MainChain::HeaviestChain");
    std::lock_guard<fetch::mutex::Mutex> lock(main_mutex_);

    std::vector<BlockType> result;

    auto topBlock = blockChain_.at(heaviest_.second);

    while ((topBlock.body().block_number != 0) && (result.size() < limit))
    {
      result.push_back(topBlock);
      auto hash = topBlock.body().previous_hash;

      // Walk down
      auto it = blockChain_.find(hash);
      if (it == blockChain_.end())
      {
        FETCH_LOG_INFO(LOGGING_NAME,
                       "Mainchain: Failed while walking down\
            from top block to find genesis!");
        break;
      }

      topBlock = (*it).second;
    }

    result.push_back(topBlock);  // this should be genesis
    return result;
  }

  void reset()
  {
    std::lock_guard<fetch::mutex::Mutex> lock_main(main_mutex_);
    std::lock_guard<fetch::mutex::Mutex> lock_loose(loose_mutex_);

    blockChain_.clear();
    tips_.clear();
    looseBlocks_.clear();

    {
      std::ostringstream fileMain;
      std::ostringstream fileIndex;

      fileMain << "chain_" << minerNumber_ << ".db";
      fileIndex << "chainIndex_" << minerNumber_ << ".db";

      blockStore_.New(fileMain.str(), fileIndex.str());
    }

    // recreate genesis
    BlockType genesis;
    genesis.UpdateDigest();
    // genesis.body().hash = byte_array::FromBase64("+++++++++++++++++Genesis+++++++++++++++++++=");
    // genesis.body().previous_ha

    // Add genesis to block chain
    genesis.loose()             = false;
    blockChain_[genesis.hash()] = genesis;

    // Create tip
    auto tip              = std::make_shared<Tip>();
    tip->total_weight     = genesis.weight();
    tips_[genesis.hash()] = std::move(tip);
    heaviest_             = std::make_pair(genesis.weight(), genesis.hash());
  }

  bool Get(BlockHash hash, BlockType &block) const
  {
    std::lock_guard<fetch::mutex::Mutex> lock(main_mutex_);

    auto it = blockChain_.find(hash);

    if (it != blockChain_.end())
    {
      block = (*it).second;
      return true;
    }

    return false;
  }

private:
  // Long term storage and backup
  fetch::storage::ObjectStore<BlockType> blockStore_;
  const uint32_t                         blockConfirmation_ = 10;
  const uint32_t                         minerNumber_;
  bool                                   saving_to_file_ = false;

  mutable fetch::mutex::Mutex                         main_mutex_{__LINE__, __FILE__};
  std::unordered_map<BlockHash, BlockType>            blockChain_;  // all recent blocks are here
  std::unordered_map<BlockHash, std::shared_ptr<Tip>> tips_;        // Keep track of the tips
  std::pair<uint64_t, BlockHash>                      heaviest_;    // Heaviest block/tip

  mutable fetch::mutex::Mutex                          loose_mutex_{__LINE__, __FILE__};
  std::unordered_map<PrevHash, std::vector<BlockHash>> looseBlocks_;  // Waiting (loose) blocks

  void RecoverFromFile()
  {
    std::ostringstream fileMain;
    std::ostringstream fileIndex;

    fileMain << "chain_" << minerNumber_ << ".db";
    fileIndex << "chainIndex_" << minerNumber_ << ".db";

    blockStore_.Load(fileMain.str(), fileIndex.str());

    BlockType block;
    if (blockStore_.Get(storage::ResourceAddress("head"), block))
    {
      block.UpdateDigest();
      AddBlock(block);

      while (blockStore_.Get(storage::ResourceID(block.body().previous_hash), block))
      {
        block.UpdateDigest();
        AddBlock(block);
      }
    }
  }

  void WriteToFile()
  {
    fetch::generics::MilliTimer myTimer("MainChain::WriteToFile");
    return;  // TODO(remove before flight)
    if (!saving_to_file_)
    {
      return;
    }

    // Add confirmed blocks to file
    BlockType block  = blockChain_.at(heaviest_.second);
    bool      failed = false;

    // Find the block N back from our heaviest
    for (std::size_t i = 0; i < blockConfirmation_; ++i)
    {
      if (!GetPrev(block))
      {
        failed = true;
        break;
      }
    }

    // This block is now the head in our file
    if (!failed)
    {
      blockStore_.Set(storage::ResourceAddress("head"), block);
      blockStore_.Set(storage::ResourceID(block.hash()), block);
    }

    // Walk down the file to check we have an unbroken chain
    while (GetPrev(block) && !blockStore_.Has(storage::ResourceID(block.hash())))
    {
      blockStore_.Set(storage::ResourceID(block.hash()), block);
    }

    // Clear the block from ram
    FlushBlock(block);
  }

  void FlushBlock(BlockType const &block)
  {
    {
      auto it = blockChain_.find(block.hash());

      if (it != blockChain_.end())
      {
        blockChain_.erase(it);
      }
    }

    auto it = tips_.find(block.hash());

    if (it != tips_.end())
    {
      tips_.erase(it);
    }
  }

  bool GetPrev(BlockType &block)
  {

    if (block.body().previous_hash == block.hash())
    {
      return false;
    }

    auto it = blockChain_.find(block.body().previous_hash);

    if (it == blockChain_.end())
    {
      return false;
    }

    block = (*it).second;

    return true;
  }

  bool GetPrevFromStore(BlockType &block)
  {
    if (block.body().previous_hash == block.hash())
    {
      return false;
    }

    return blockStore_.Get(storage::ResourceID(block.body().previous_hash), block);
  }

  // We have added a non-loose block. It is then safe to lock the loose blocks map and
  // walk through it adding the blocks, so long as we do breadth first search (!!)
  void CompleteLooseBlocks(BlockType const &block)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(loose_mutex_);

    auto it = looseBlocks_.find(block.hash());
    if (it == looseBlocks_.end())
    {
      return;
    }

    std::vector<BlockHash> blocks_to_add = (*it).second;
    looseBlocks_.erase(it);

    FETCH_LOG_DEBUG(LOGGING_NAME,
                    "Found loose blocks completed by addition of block: ", blocks_to_add.size());

    while (blocks_to_add.size() > 0)
    {
      std::vector<BlockHash> next_blocks_to_add;

      // This is the breadth search, for each block to add, add it, next_blocks_to_add will
      // get pushed with the next layer of blocks
      for (auto const &hash : blocks_to_add)
      {
        // This should be guaranteed safe
        BlockType addBlock = blockChain_.at(hash);

        // This won't re-call this function due to the flag
        AddBlock(addBlock, true);

        // The added block was not loose. Continue to clear
        auto it = looseBlocks_.find(addBlock.hash());
        if (it != looseBlocks_.end())
        {
          std::vector<BlockHash> const &next_blocks = (*it).second;

          for (auto const &block_hash : next_blocks)
          {
            next_blocks_to_add.push_back(block_hash);
          }
          looseBlocks_.erase(it);
        }
      }

      blocks_to_add = std::move(next_blocks_to_add);
    }
  }

  void NewLooseBlock(BlockType &block)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(loose_mutex_);
    // Get vector of waiting blocks and push ours on
    auto &waitingBlocks = looseBlocks_[block.body().previous_hash];
    waitingBlocks.push_back(block.hash());
    block.loose()             = true;
    blockChain_[block.hash()] = block;
  }

  // Case where the block prev isn't found, need to check back in history, and add the prev to
  // our cache. This might be expensive due to disk reads and hashing.
  bool CheckDiskForBlock(BlockType &block)
  {
    // Only guaranteed way to calculate the weight of the block is to walk back to genesis
    // is by walking backwards from one of our tips
    BlockType walk_block;
    BlockType prev_block;

    if (!blockStore_.Get(storage::ResourceID(block.body().previous_hash), prev_block))
    {
      std::lock_guard<fetch::mutex::Mutex> lock(main_mutex_);
      FETCH_LOG_DEBUG(LOGGING_NAME, "Didn't find block's previous, adding as loose block");
      NewLooseBlock(block);
      return false;
    }

    // The previous block is in our object store but we don't know its weight, need to recalculate
    walk_block            = prev_block;
    uint64_t total_weight = walk_block.totalWeight();

    // walk block should reach genesis walking back
    while (GetPrevFromStore(walk_block))
    {
      total_weight += walk_block.weight();
    }

    // We should now have an up to date prev block from file, put it in our cached blockchain and
    // re-add
    FETCH_LOG_DEBUG(LOGGING_NAME, "Reviving block from file");
    {
      std::lock_guard<fetch::mutex::Mutex> lock(main_mutex_);
      prev_block.totalWeight()       = total_weight;
      prev_block.loose()             = false;
      blockChain_[prev_block.hash()] = prev_block;
    }

    return AddBlock(block);
  }

  bool UpdateTips(BlockType &block, BlockType const &prev_block)
  {
    // Check whether blocks prev hash refers to a valid tip (common case)
    std::shared_ptr<Tip> tip;
    auto                 tipRef           = tips_.find(block.body().previous_hash);
    bool                 heaviestAdvanced = false;

    if (tipRef != tips_.end())
    {
      // To advance a tip, need to update its BlockHash in the map
      tip = ((*tipRef).second);
      tips_.erase(tipRef);
      tip->total_weight += block.weight();
      block.totalWeight() = tip->total_weight;
      block.loose()       = false;

      FETCH_LOG_DEBUG(LOGGING_NAME, "Pushing block onto already existing tip:");

      // Update heaviest pointer if necessary
      if ((tip->total_weight > heaviest_.first) ||
          ((tip->total_weight == heaviest_.first) && (block.hash() > heaviest_.second)))
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "Updating heaviest with tip");

        heaviest_.first  = tip->total_weight;
        heaviest_.second = block.hash();

        heaviestAdvanced = true;
      }
    }
    else  // Didn't find a corresponding tip
    {
      // We are not building on a tip, create a new tip
      FETCH_LOG_DEBUG(LOGGING_NAME, "Received new block with no corresponding tip");

      block.totalWeight() = block.weight() + prev_block.totalWeight();
      tip                 = std::make_shared<Tip>();
      tip->total_weight   = block.totalWeight();

      // Check if this advanced the heaviest tip
      if ((tip->total_weight > heaviest_.first) ||
          ((tip->total_weight == heaviest_.first) && (block.hash() > heaviest_.second)))
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "creating new tip that is now heaviest! (new fork)");

        heaviest_.first  = tip->total_weight;
        heaviest_.second = block.hash();
        heaviestAdvanced = true;
      }
    }

    if (tip)
    {
      tips_[block.hash()] = tip;
    }

    return heaviestAdvanced;
  }
};

}  // namespace chain
}  // namespace fetch
