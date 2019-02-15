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

#include "ledger/chain/main_chain.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/encoders.hpp"

#include <utility>

using fetch::byte_array::ToBase64;

namespace fetch {
namespace ledger {

/**
 * Constructs the main chain
 *
 * @param disable_persistence Flag to signal is persistence should be disabled
 */
MainChain::MainChain(bool disable_persistence)
{
  RLock lock(main_mutex_);

  // create the genesis block and add it to the cache
  Block genesis                   = CreateGenesisBlock();
  block_chain_[genesis.body.hash] = genesis;

  // add the tip for this block
  AddTip(genesis);

  if (!disable_persistence)
  {
    // for the moment the file reading is not supported
    throw std::runtime_error("Unsupported behaviour currently");

    // create the block store
    block_store_ = std::make_unique<BlockStore>();

    RecoverFromFile();
  }
}

/**
 * Adds a block to the chain
 *
 * @param block The block that will be added to the chain. The block is populated with metadata
 * during insert
 * @return true if block successfully added, otherwise false
 */
bool MainChain::AddBlock(Block &block)
{
  // update the weight based on the proof and the number of transactions
  for (auto const &slice : block.body.slices)
  {
    block.weight += slice.size();
  }

  bool const success = AddBlockInternal(block, false);
  FETCH_LOG_WARN(LOGGING_NAME, "New Block: ", ToBase64(block.body.hash), " -> ", block.weight,
                 " -> ", block.total_weight);

  return success;
}

/**
 * Get the current heaviest block on the chain
 * @return The reference to the heaviest block
 */
Block const &MainChain::HeaviestBlock() const
{
  RLock lock(main_mutex_);
  return block_chain_.at(heaviest_.hash);
}

bool MainChain::InvalidateTip(BlockHash hash)
{
  bool success{false};

  RLock lock(main_mutex_);

  // find the tip
  auto tip_it = tips_.find(hash);
  if (tip_it != tips_.end())
  {
    Block block{};

    // lookup both the current block and its previous block
    if (LookupBlock(hash, block) /*&& LookupBlock(block.body.previous_hash, previous_block)*/)
    {
      // create a new tip structure to replace the old one
      auto new_tip = std::make_shared<Tip>();

      assert(tip_it->second->total_weight > block.weight);
      new_tip->total_weight = tip_it->second->total_weight - block.weight;

      // erase the old tip
      tips_.erase(tip_it);

      // add the new tip
      tips_[block.body.previous_hash] = new_tip;

      // update the heaviest tip
      success = DetermineHeaviestTip();
    }
  }

  return success;
}

bool MainChain::InvalidateBlock(BlockHash hash)
{
  using BlockHashSet = std::unordered_set<BlockHash>;

  bool success{false};

  RLock lock_main(main_mutex_);
  RLock lock_loose(loose_mutex_);

  // Check that the input block hash is actually in the cache
  auto invalid_block_it = block_chain_.find(hash);
  if (block_chain_.end() != invalid_block_it)
  {
    BlockHashSet invalidated_blocks{};

    // add the original element into the invalidated block set
    invalidated_blocks.insert(hash);
    block_chain_.erase(hash);

    // Step 1. Evaluate all the blocks which have been now been made invalid from this change
    for (;;)
    {
      // find the next element to remove
      auto it = std::find_if(block_chain_.begin(), block_chain_.end(),
                             [&invalidated_blocks, hash](BlockMap::value_type const &e) {
                               return (invalidated_blocks.find(e.second.body.previous_hash) !=
                                       invalidated_blocks.end());
                             });

      // once we have done a complete sweep where we haven't found anything, the fallout has been
      // calculated and collected
      if (it == block_chain_.end())
      {
        break;
      }

      // update our removed blocks and hashes
      invalidated_blocks.insert(it->first);

      // remove the element from the map
      block_chain_.erase(it);
    }

    // Step 2. Loop through all the loose blocks and remove any references to invalidated blocks
    auto waiting_blocks_it = loose_blocks_.begin();
    while (waiting_blocks_it != loose_blocks_.end())
    {
      auto &hash_array = waiting_blocks_it->second;

      // iterate over the hash arrays removing entries that match
      auto hash_it = hash_array.begin();
      while (hash_it != hash_array.end())
      {
        if (invalidated_blocks.find(*hash_it) != invalidated_blocks.end())
        {
          // remove the entry from the hash array
          hash_it = hash_array.erase(hash_it);
        }
        else
        {
          // advance to next hash
          ++hash_it;
        }
      }

      // finally, if our actions have emptied the hash array we must remove the entry from the map
      if (hash_array.empty())
      {
        // remove the empty array
        waiting_blocks_it = loose_blocks_.erase(waiting_blocks_it);
      }
      else
      {
        // advance to the next one
        ++waiting_blocks_it;
      }
    }

    // Step 3. Since we might have removed a whole series of blocks the tips datastructure has been
    // invalidated. We need to evaluate the changes here
    success = ReindexTips();
  }

  return success;
}

/**
 * Walk the block history starting from the heaviest block
 *
 * @param limit The maximum number of blocks to be returned
 * @return The array of blocks
 * @throws std::runtime_error if a block lookup occurs
 */
MainChain::Blocks MainChain::HeaviestChain(uint64_t limit) const
{
  fetch::generics::MilliTimer myTimer("MainChain::HeaviestChain");
  RLock                       lock(main_mutex_);

  return ChainPreceding(heaviest_block_hash(), limit);
}

/**
 * Walk the block history collecting blocks until
 *
 * @param start The hash of the first block
 * @param limit The maximum number of blocks to be returned
 * @return The array of blocks
 * @throws std::runtime_error if a block lookup occurs
 */
MainChain::Blocks MainChain::ChainPreceding(BlockHash start, uint64_t limit) const
{
  fetch::generics::MilliTimer myTimer("MainChain::ChainPreceding");
  RLock                       lock(main_mutex_);

  Blocks result;

  // lookup the heaviest block hash
  Block     block;
  BlockHash current_hash = std::move(start);

  while (result.size() < limit)
  {
    // exit once we have reached genesis
    if (GENESIS_DIGEST == current_hash)
    {
      break;
    }

    // lookup the block
    if (!LookupBlock(current_hash, block))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Block lookup failure for block: ", ToBase64(current_hash));
      throw std::runtime_error("Failed to lookup block");
    }

    // update the results
    result.push_back(block);

    // walk the hash
    current_hash = block.body.previous_hash;
  }

  return result;
}

/**
 * Get a common sub tree from the chain.
 *
 * This function will search down both input references input nodes until a common ancestor is
 * found. Once found the blocks from the specified tip that to that common ancestor are returned.
 *
 * @param blocks The output list of blocks (from heaviest to lightest)
 * @param tip The tip the output list should start from
 * @param node A node in chain that is searched for
 * @param limit The maximum number of nodes to be returned
 * @return true if successful, otherwise false
 */
bool MainChain::GetPathToCommonAncestor(Blocks &blocks, BlockHash tip, BlockHash node,
                                        uint64_t limit)
{
  fetch::generics::MilliTimer myTimer("MainChain::GetPathToCommonAncestor");
  RLock                       lock(main_mutex_);

  bool success{false};

  // clear the output structure
  blocks.clear();

  Block left;
  Block right;

  BlockHash left_hash  = std::move(tip);
  BlockHash right_hash = std::move(node);

  // The algorithm implemented here is effectively a coordinated parallel walk about from the two
  // input tips until the a common ancestor is located.
  while (blocks.size() < limit)
  {
    // load up the left side
    if (left.body.hash != left_hash)
    {
      if (!Get(left_hash, left))
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup block (left): ", ToBase64(left_hash));
        break;
      }

      // left side always loaded into output queue as we traverse
      blocks.push_back(left);
    }

    // load up the right side
    if (right.body.hash != right_hash)
    {
      if (!Get(right_hash, right))
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup block (right): ", ToBase64(right_hash));
        break;
      }
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Left: ", ToBase64(left_hash), " -> ", left.body.block_number,
                   " Right: ", ToBase64(right_hash), " -> ", right.body.block_number);

    if (left_hash == right_hash)
    {
      // exit condition
      success = true;
      break;
    }
    else
    {
      if (left.body.block_number <= right.body.block_number)
      {
        right_hash = right.body.previous_hash;
      }

      if (left.body.block_number >= right.body.block_number)
      {
        left_hash = left.body.previous_hash;
      }
    }
  }

  return success;
}

// TODO(EJF): Remove this function, not needed
void MainChain::Reset()
{
  RLock lock_main(main_mutex_);
  RLock lock_loose(loose_mutex_);

  block_chain_.clear();
  tips_.clear();
  loose_blocks_.clear();

  if (block_store_)
  {
    block_store_->New("chain.db", "chain.index.db");
  }

  // recreate genesis
  Block genesis                   = CreateGenesisBlock();
  block_chain_[genesis.body.hash] = genesis;

  // Create tip
  auto tip                 = std::make_shared<Tip>();
  tip->total_weight        = genesis.weight;
  tips_[genesis.body.hash] = std::move(tip);
  heaviest_.Update(genesis.weight, genesis.body.hash);
}

bool MainChain::Get(BlockHash hash, Block &block) const
{
  RLock lock(main_mutex_);

  return LookupBlock(hash, block);
}

MainChain::BlockHashs MainChain::GetMissingBlockHashes(std::size_t maximum) const
{
  BlockHashs results;

  for (auto const &loose_block : loose_blocks_)
  {
    if (maximum <= results.size())
    {
      break;
    }
    results.push_back(loose_block.first);
  }

  return results;
}

bool MainChain::HasMissingBlocks() const
{
  RLock lock(loose_mutex_);
  return !loose_blocks_.empty();
}

MainChain::BlockHashSet MainChain::GetTips() const
{
  RLock lock(loose_mutex_);

  BlockHashSet hash_set;
  for (auto const &element : tips_)
  {
    hash_set.insert(element.first);
  }

  return hash_set;
}

void MainChain::RecoverFromFile()
{
  assert(static_cast<bool>(block_store_));

  block_store_->Load("chain.db", "chain.index.db");

  Block block;
  if (block_store_->Get(storage::ResourceAddress("head"), block))
  {
    block.UpdateDigest();
    AddBlockInternal(block, false);

    while (block_store_->Get(storage::ResourceID(block.body.previous_hash), block))
    {
      block.UpdateDigest();
      AddBlockInternal(block, false);
    }
  }
}

void MainChain::WriteToFile()
{
  // skip if the block store is not persistent
  if (!block_store_)
  {
    return;
  }

  fetch::generics::MilliTimer myTimer("MainChain::WriteToFile");

  if (block_store_)
  {
    // Add confirmed blocks to file
    Block block  = block_chain_.at(heaviest_.hash);
    bool  failed = false;

    // Find the block N back from our heaviest
    for (std::size_t i = 0; i < block_confirmation_; ++i)
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
      block_store_->Set(storage::ResourceAddress("head"), block);
      block_store_->Set(storage::ResourceID(block.body.hash), block);
    }

    // Walk down the file to check we have an unbroken chain
    while (GetPrev(block) && !block_store_->Has(storage::ResourceID(block.body.hash)))
    {
      block_store_->Set(storage::ResourceID(block.body.hash), block);
    }

    // Clear the block from ram
    FlushBlock(block);
  }
}

void MainChain::FlushBlock(Block const &block)
{
  // remove the block from the block map
  block_chain_.erase(block.body.hash);

  // remove the block hash from the tips
  tips_.erase(block.body.hash);
}

bool MainChain::GetPrev(Block &block)
{
  if (block.body.previous_hash == block.body.hash)
  {
    return false;
  }

  auto it = block_chain_.find(block.body.previous_hash);
  if ((block_chain_.end() == it) && block_store_)
  {
    // Fallback - look in storage for block
    return block_store_->Get(storage::ResourceID(block.body.previous_hash), block);
  }

  block = it->second;

  return true;
}

bool MainChain::GetPrevFromStore(Block &block)
{
  assert(static_cast<bool>(block_store_));

  if (block.body.previous_hash == block.body.hash)
  {
    return false;
  }

  return block_store_->Get(storage::ResourceID(block.body.previous_hash), block);
}

// We have added a non-loose block. It is then safe to lock the loose blocks map and
// walk through it adding the blocks, so long as we do breadth first search (!!)
void MainChain::CompleteLooseBlocks(Block const &block)
{
  RLock lock_main(main_mutex_);
  RLock lock(loose_mutex_);

  auto it = loose_blocks_.find(block.body.hash);
  if (it == loose_blocks_.end())
  {
    return;
  }

  BlockHashs blocks_to_add = it->second;
  loose_blocks_.erase(it);

  FETCH_LOG_DEBUG(LOGGING_NAME,
                  "Found loose blocks completed by addition of block: ", blocks_to_add.size());

  while (!blocks_to_add.empty())
  {
    BlockHashs next_blocks_to_add{};

    // This is the breadth search, for each block to add, add it, next_blocks_to_add will
    // get pushed with the next layer of blocks
    for (auto const &hash : blocks_to_add)
    {
      // This should be guaranteed safe
      Block add_block = block_chain_.at(hash);

      // This won't re-call this function due to the flag
      AddBlockInternal(add_block, true);

      // The added block was not loose. Continue to clear
      auto it = loose_blocks_.find(add_block.body.hash);
      if (it != loose_blocks_.end())
      {
        BlockHashs const &next_blocks = it->second;

        for (auto const &block_hash : next_blocks)
        {
          next_blocks_to_add.push_back(block_hash);
        }

        loose_blocks_.erase(it);
      }
    }

    blocks_to_add = std::move(next_blocks_to_add);
  }
}

/**
 * Records the input block as loose
 *
 * @param block The reference to the block being marked as loose
 */
void MainChain::RecordLooseBlock(Block &block)
{
  RLock lock(loose_mutex_);

  // Get vector of waiting blocks and push ours on
  auto &waiting_blocks = loose_blocks_[block.body.previous_hash];
  waiting_blocks.push_back(block.body.hash);

  block.is_loose                = true;
  block_chain_[block.body.hash] = block;
}

// Case where the block prev isn't found, need to check back in history, and add the prev to
// our cache. This might be expensive due to disk reads and hashing.
bool MainChain::CheckDiskForBlock(Block &block)
{
  if (!block_store_)
  {
    RecordLooseBlock(block);
    return true;
  }

  // Only guaranteed way to calculate the weight of the block is to walk back to genesis
  // is by walking backwards from one of our tips
  Block walk_block{};
  Block prev_block{};

  if (!block_store_->Get(storage::ResourceID(block.body.previous_hash), prev_block))
  {
    RLock lock(main_mutex_);
    FETCH_LOG_DEBUG(LOGGING_NAME, "Didn't find block's previous, adding as loose block");
    RecordLooseBlock(block);
    return true;
  }

  // The previous block is in our object store but we don't know its weight, need to recalculate
  walk_block            = prev_block;
  uint64_t total_weight = walk_block.total_weight;

  // walk block should reach genesis walking back
  while (GetPrevFromStore(walk_block))
  {
    total_weight += walk_block.weight;
  }

  // We should now have an up to date prev block from file, put it in our cached blockchain and
  // re-add
  FETCH_LOG_DEBUG(LOGGING_NAME, "Reviving block from file");
  {
    RLock lock(main_mutex_);

    prev_block.total_weight            = total_weight;
    prev_block.is_loose                = false;
    block_chain_[prev_block.body.hash] = prev_block;
  }

  return AddBlockInternal(block, false);
}

bool MainChain::UpdateTips(Block &block, Block const &prev_block)
{
  // Check whether blocks prev hash refers to a valid tip (common case)
  TipPtr tip{};

  auto tip_ref           = tips_.find(block.body.previous_hash);
  bool heaviest_advanced = false;

  if (tip_ref != tips_.end())
  {
    // To advance a tip, need to update its BlockHash in the map
    tip = ((*tip_ref).second);
    tips_.erase(tip_ref);
    tip->total_weight += block.weight;

    block.total_weight = tip->total_weight;
    block.is_loose     = false;

    FETCH_LOG_DEBUG(LOGGING_NAME, "Pushing block onto already existing tip:");

    // Update heaviest pointer if necessary
    if ((tip->total_weight > heaviest_.weight) ||
        ((tip->total_weight == heaviest_.weight) && (block.body.hash > heaviest_.hash)))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Updating heaviest with tip");

      heaviest_.Update(tip->total_weight, block.body.hash);

      heaviest_advanced = true;
    }
  }
  else  // Didn't find a corresponding tip
  {
    // We are not building on a tip, create a new tip
    FETCH_LOG_DEBUG(LOGGING_NAME, "Received new block with no corresponding tip");

    block.total_weight = block.weight + prev_block.total_weight;
    tip                = std::make_shared<Tip>();
    tip->total_weight  = block.total_weight;

    // Check if this advanced the heaviest tip
    if ((tip->total_weight > heaviest_.weight) ||
        ((tip->total_weight == heaviest_.weight) && (block.body.hash > heaviest_.hash)))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "creating new tip that is now heaviest! (new fork)");

      heaviest_.Update(tip->total_weight, block.body.hash);
      heaviest_advanced = true;
    }
  }

  if (tip)
  {
    tips_[block.body.hash] = tip;
  }

  return heaviest_advanced;
}

bool MainChain::AddBlockInternal(Block &block, bool recursive)
{
  assert(block.body.previous_hash.size() > 0);

  fetch::generics::MilliTimer myTimer("MainChain::AddBlock");
  RLock                       lock(main_mutex_);

  if (block.body.hash.empty())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "MainChain: AddBlock called with no digest");
    return false;
  }

  Block prev_block;

  if (!recursive)
  {
    // First check if block already exists (not checking in object store)
    if (block_chain_.find(block.body.hash) != block_chain_.end())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Attempting to add already seen block");
      return false;
    }

    // Look for the prev block to this one
    auto it_prev = block_chain_.find(block.body.previous_hash);

    if (it_prev == block_chain_.end())
    {
      // Note: think about rolling back into FS
      FETCH_LOG_INFO(LOGGING_NAME,
                     "Block prev not found: ", byte_array::ToBase64(block.body.previous_hash));

      // We can't find the prev, this is probably a loose block.
      return CheckDiskForBlock(block);
    }

    prev_block = (*it_prev).second;

    // Also a loose block if it points to a loose block
    if (prev_block.is_loose)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Block connects to loose block");
      RecordLooseBlock(block);
      return true;
    }
  }
  else  // else of recursive==true
  {
    auto it = block_chain_.find(block.body.previous_hash);
    if (it == block_chain_.end())
    {
      return false;
    }

    prev_block = it->second;
  }

  // At this point we have a new block with a prev that's known and not loose. Update tips
  block.is_loose        = false;
  bool heaviestAdvanced = UpdateTips(block, prev_block);

  // Add block
  FETCH_LOG_DEBUG(LOGGING_NAME, "Adding block to chain: ", ToBase64(block.hash()));
  block_chain_[block.body.hash] = block;

  if (heaviestAdvanced)
  {
    WriteToFile();
  }

  // Now we're done, it's possible this added block completed some loose blocks.
  if (!recursive)
  {
    CompleteLooseBlocks(block);
  }

  return true;
}

/**
 * Attempt to lookup a block.
 *
 * The search is performed initially on the in memory cache and then if this fails the persistent
 * disk storage is searched
 *
 * @param hash The hash of the block to search for
 * @param block The output block to be populated
 * @return true if successful, otherwise false
 */
bool MainChain::LookupBlock(BlockHash hash, Block &block) const
{
  return LookupBlockFromCache(hash, block) || LookupBlockFromStorage(hash, block);
}

/**
 * Attempt to locate a block stored in the im memory cache
 *
 * @param hash The hash of the block to search for
 * @param block The output block to be populated
 * @return true if successful, otherwise false
 */
bool MainChain::LookupBlockFromCache(BlockHash hash, Block &block) const
{
  bool success{false};

  RLock lock(main_mutex_);

  // perform the lookup
  auto const it = block_chain_.find(hash);
  if ((block_chain_.end() != it))
  {
    block   = it->second;
    success = true;
  }

  return success;
}

/**
 * Attempt to locate a block stored in the persistent object store
 *
 * @param hash The hash of the block to search for
 * @param block The output block to be populated
 * @return true if successful, otherwise false
 */
bool MainChain::LookupBlockFromStorage(BlockHash hash, Block &block) const
{
  bool success{false};

  if (block_store_)
  {
    success = block_store_->Get(storage::ResourceID(std::move(hash)), block);
  }

  return success;
}

/**
 * Adds a new tip for the given block
 *
 * @param block The block for the tip to be created from
 */
bool MainChain::AddTip(Block const &block)
{
  RLock lock(main_mutex_);

  auto tip               = std::make_shared<Tip>();
  tip->total_weight      = block.total_weight;
  tips_[block.body.hash] = tip;

  return DetermineHeaviestTip();
}

/**
 * Evaluate the current set of tips in the chain and determine which of them is the heaviest
 *
 * @return true if successful, otherwise false
 */
bool MainChain::DetermineHeaviestTip()
{
  bool  success{false};
  RLock lock(main_mutex_);

  if (!tips_.empty())
  {
    // find the heaviest item in our tip selection
    auto it = std::max_element(
        tips_.begin(), tips_.end(), [](TipsMap::value_type const &a, TipsMap::value_type const &b) {
          bool const is_heavier      = a.second->total_weight < b.second->total_weight;
          bool const is_equal_weight = a.second->total_weight == b.second->total_weight;
          bool const is_hash_larger  = a.first < b.first;

          return (is_heavier || (is_equal_weight && is_hash_larger));
        });

    // update the heaviest
    heaviest_.hash   = it->first;
    heaviest_.weight = it->second->total_weight;
    success          = true;
  }

  return success;
}

/**
 * Reindex the tips
 *
 * This is a currently an expensive operation which will scale in the order N(1 + logN) in the worst
 * case.
 *
 * @return true if successful, otherwise false
 */
bool MainChain::ReindexTips()
{
  RLock lock(main_mutex_);

  // Step 1. Generate a new set of blocks (so that we can order them)
  std::list<Block> block_list{};
  for (auto const &block : block_chain_)
  {
    block_list.push_back(block.second);
  }

  // Step 2. Order the blocks so that we have a good order for them to be evaluated
  block_list.sort([](Block const &a, Block const &b) {
    if (a.body.block_number == b.body.block_number)
    {
      return (a.body.hash < b.body.hash);
    }
    else
    {
      return (a.body.block_number < b.body.block_number);
    }
  });

  // Step 3. Loop through ordered blocks and generate the new list of tips
  BlockHashSet tips{};
  for (auto const &block : block_list)
  {
    tips.erase(block.body.previous_hash);
    tips.insert(block.body.hash);
  }
  block_list.clear();  // free some memory

  // Step 4. Recreate the tips map index
  TipsMap new_tips{};
  Block   block;
  for (auto const &tip : tips)
  {
    if (!LookupBlockFromCache(tip, block))
    {
      return false;
    }

    // add update the new tip
    new_tips[tip] = std::make_shared<Tip>(block.total_weight);
  }

  // clear the existing tips
  tips_ = std::move(new_tips);

  // finally update the heaviest tip
  return DetermineHeaviestTip();
}

/**
 * Create the initial genesis block
 * @return The generated block
 */
Block MainChain::CreateGenesisBlock()
{
  Block genesis{};
  genesis.body.previous_hash = GENESIS_DIGEST;
  genesis.is_loose           = false;
  genesis.body.merkle_hash   = GENESIS_MERKLE_ROOT;

  genesis.UpdateDigest();

  return genesis;
}

}  // namespace ledger
}  // namespace fetch
