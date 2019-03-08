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
 * Converts a block status into a human readable string
 *
 * @param status The status enumeration
 * @return The output text
 */
char const *ToString(BlockStatus status)
{
  char const *text = "Unknown";

  switch (status)
  {
  case BlockStatus::ADDED:
    text = "Added";
    break;
  case BlockStatus ::LOOSE:
    text = "Loose";
    break;
  case BlockStatus::DUPLICATE:
    text = "Duplicate";
    break;
  case BlockStatus ::INVALID:
    text = "Invalid";
    break;
  }

  return text;
}

/**
 * Constructs the main chain
 *
 * @param mode Flag to signal which storage mode has been requested
 */
MainChain::MainChain(Mode mode)
{
  RLock lock(main_mutex_);

  // create the genesis block and add it to the cache
  auto genesis = CreateGenesisBlock();

  // add the block to the cache
  AddBlockToCache(genesis);

  // add the tip for this block
  AddTip(genesis);

  if (Mode::IN_MEMORY_DB != mode)
  {
    // create the block store
    block_store_ = std::make_unique<BlockStore>();

    RecoverFromFile(mode);
  }
}

/**
 * Adds a block to the chain
 *
 * @param block The block that will be added to the chain.
 * @return The status enumeration signalling the operation result
 */
BlockStatus MainChain::AddBlock(Block const &blk)
{
  // create a copy of the block
  auto block = std::make_shared<Block>(blk);

  // update the weight based on the proof and the number of transactions
  block->weight       = 1;
  block->total_weight = 1;
  for (auto const &slice : block->body.slices)
  {
    block->weight += slice.size();
  }

  // pass the block to the
  auto const status = InsertBlock(block);
  FETCH_LOG_DEBUG(LOGGING_NAME, "New Block: ", ToBase64(block->body.hash), " -> ", ToString(status),
                  " (weight: ", block->weight, " total: ", block->total_weight, ")");

  return status;
}

/**
 * Get the current heaviest block on the chain
 *
 * @return The reference to the heaviest block
 */
MainChain::BlockPtr MainChain::GetHeaviestBlock() const
{
  RLock lock(main_mutex_);
  return GetBlock(heaviest_.hash);
}

/**
 * Removes the block (and all blocks ahead of it) from the chain
 *
 * @param hash The has to be removed
 * @return True if successful, otherwise false
 */
bool MainChain::RemoveBlock(BlockHash hash)
{
  // TODO(private issue 666): Improve performance of block removal

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
                               return (invalidated_blocks.find(e.second->body.previous_hash) !=
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
MainChain::Blocks MainChain::GetHeaviestChain(uint64_t limit) const
{
  fetch::generics::MilliTimer myTimer("MainChain::HeaviestChain");
  RLock                       lock(main_mutex_);

  return GetChainPreceding(GetHeaviestBlockHash(), limit);
}

/**
 * Walk the block history collecting blocks until either genesis or the block limit is reached
 *
 * @param start The hash of the first block
 * @param limit The maximum number of blocks to be returned
 * @return The array of blocks
 * @throws std::runtime_error if a block lookup occurs
 */
MainChain::Blocks MainChain::GetChainPreceding(BlockHash start, uint64_t limit) const
{
  fetch::generics::MilliTimer myTimer("MainChain::ChainPreceding");
  RLock                       lock(main_mutex_);

  Blocks result;

  // lookup the heaviest block hash
  BlockPtr  block;
  BlockHash current_hash = std::move(start);

  while (result.size() < limit)
  {
    // exit once we have reached genesis
    if (GENESIS_DIGEST == current_hash)
    {
      break;
    }

    // lookup the block
    block = GetBlock(current_hash);
    if (!block)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Block lookup failure for block: ", ToBase64(current_hash));
      throw std::runtime_error("Failed to lookup block");
    }

    // update the results
    result.push_back(block);

    // walk the hash
    current_hash = block->body.previous_hash;
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
                                        uint64_t limit) const
{
  fetch::generics::MilliTimer myTimer("MainChain::GetPathToCommonAncestor");
  RLock                       lock(main_mutex_);

  bool success{false};

  // clear the output structure
  blocks.clear();

  BlockPtr left{};
  BlockPtr right{};

  BlockHash left_hash  = std::move(tip);
  BlockHash right_hash = std::move(node);

  // The algorithm implemented here is effectively a coordinated parallel walk about from the two
  // input tips until the a common ancestor is located.
  while (blocks.size() < limit)
  {
    // load up the left side
    if (!left || left->body.hash != left_hash)
    {
      left = GetBlock(left_hash);
      if (!left)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup block (left): ", ToBase64(left_hash));
        break;
      }

      // left side always loaded into output queue as we traverse
      blocks.push_back(left);
    }

    // load up the right side
    if (!right || right->body.hash != right_hash)
    {
      right = GetBlock(right_hash);
      if (!right)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup block (right): ", ToBase64(right_hash));
        break;
      }
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Left: ", ToBase64(left_hash), " -> ", left->body.block_number,
                    " Right: ", ToBase64(right_hash), " -> ", right->body.block_number);

    if (left_hash == right_hash)
    {
      // exit condition
      success = true;
      break;
    }
    else
    {
      if (left->body.block_number <= right->body.block_number)
      {
        right_hash = right->body.previous_hash;
      }

      if (left->body.block_number >= right->body.block_number)
      {
        left_hash = left->body.previous_hash;
      }
    }
  }

  return success;
}

/**
 * Retrieve a block with a specific hash
 *
 * @param hash The hash being queried
 * @return The a valid shared pointer to the block if found, otherwise an empty pointer
 */
MainChain::BlockPtr MainChain::GetBlock(BlockHash hash) const
{
  RLock lock(main_mutex_);

  BlockPtr output_block{};

  // attempt to lookup the block
  auto internal_block = std::make_shared<Block>();
  if (LookupBlock(hash, internal_block))
  {
    // convert the pointer type to per const
    output_block = std::static_pointer_cast<Block const>(internal_block);
  }

  return output_block;
}

/**
 * Retrieve an array of all the missing block hashes
 *
 * @param maximum The specified maximum number of blocks to be returned
 * @return The generated array of missing hashes
 */
MainChain::BlockHashs MainChain::GetMissingBlockHashes(std::size_t maximum) const
{
  RLock lock(loose_mutex_);

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

/**
 * Determine if the chain has missing blocks
 *
 * @return true if the chain is missing blocks, otherwise false
 */
bool MainChain::HasMissingBlocks() const
{
  RLock lock(loose_mutex_);
  return !loose_blocks_.empty();
}

/**
 * Get the set of all the tips
 *
 * @return The set of tip hashes
 */
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

/**
 * Internal: Initialise and recover all previous state from the chain
 *
 * @param mode The storage mode for the chain
 */
void MainChain::RecoverFromFile(Mode mode)
{
  // TODO(private issue 667): Complete loading of chain on startup ill-advised

  assert(static_cast<bool>(block_store_));

  RLock lock(main_mutex_);

  // load the database files
  if (Mode::CREATE_PERSISTENT_DB == mode)
  {
    block_store_->New("chain.db", "chain.index.db");
  }
  else if (Mode::LOAD_PERSISTENT_DB == mode)
  {
    block_store_->Load("chain.db", "chain.index.db");
  }
  else
  {
    assert(false);
  }

  // load the head block
  IntBlockPtr block = std::make_shared<Block>();
  if (block_store_->Get(storage::ResourceAddress("head"), *block))
  {
    block->UpdateDigest();
    InsertBlock(block);

    IntBlockPtr next = std::make_shared<Block>();
    while (block_store_->Get(storage::ResourceID(block->body.previous_hash), *next))
    {
      block->UpdateDigest();

      InsertBlock(block);

      // swap the pointers
      std::swap(block, next);
    }
  }
}

/**
 * Internal: Flush confirmed blocks to disk
 */
void MainChain::WriteToFile()
{
  // skip if the block store is not persistent
  if (block_store_)
  {
    fetch::generics::MilliTimer myTimer("MainChain::WriteToFile", 500);

    // Add confirmed blocks to file
    IntBlockPtr block  = block_chain_.at(heaviest_.hash);
    bool        failed = false;

    // Find the block N back from our heaviest
    for (std::size_t i = 0; i < block_confirmation_; ++i)
    {
      if (!LookupBlock(block->body.previous_hash, block))
      {
        failed = true;
        break;
      }
    }

    // This block is now the head in our file
    if (!failed)
    {
      // store both the HEAD block as both its has and the "HEAD" entry
      block_store_->Set(storage::ResourceAddress("head"), *block);
      block_store_->Set(storage::ResourceID(block->body.hash), *block);

      FETCH_LOG_DEBUG(LOGGING_NAME, "Updating HEAD to ", ToBase64(block->body.hash));

      // Walk down the file to check we have an unbroken chain
      while (LookupBlockFromCache(block->body.previous_hash, block))
      {
        storage::ResourceID block_storage_key{block->body.hash};

        // add the block to the file storage if it
        if (!block_store_->Has(block_storage_key))
        {
          block_store_->Set(block_storage_key, *block);
        }

        // Clear the block from ram
        FlushBlock(block);
      }
    }
  }
}

/**
 * Internal: Remove the block from the cache and remove any tips
 *
 * @param block The block to be flushed
 */
void MainChain::FlushBlock(IntBlockPtr const &block)
{
  // remove the block from the block map
  block_chain_.erase(block->body.hash);

  // remove the block hash from the tips
  tips_.erase(block->body.hash);
}

// We have added a non-loose block. It is then safe to lock the loose blocks map and
// walk through it adding the blocks, so long as we do breadth first search (!!)
void MainChain::CompleteLooseBlocks(IntBlockPtr const &block)
{
  RLock lock_main(main_mutex_);
  RLock lock(loose_mutex_);

  // Determine if this block is actually a loose block, if it isn't exit immediately
  auto it = loose_blocks_.find(block->body.hash);
  if (it == loose_blocks_.end())
  {
    return;
  }

  // At this point we are sure that the current block is one of the missing blocks we are after

  // Extract the list of blocks that are waiting on this block
  BlockHashList blocks_to_add = std::move(it->second);
  loose_blocks_.erase(it);

  FETCH_LOG_DEBUG(LOGGING_NAME, blocks_to_add.size(), " are resolved from ",
                  ToBase64(block->body.hash));

  while (!blocks_to_add.empty())
  {
    BlockHashList next_blocks_to_add{};

    // This is the breadth search, for each block to add, add it, next_blocks_to_add will
    // get pushed with the next layer of blocks
    for (auto const &hash : blocks_to_add)
    {
      // This should be guaranteed safe
      IntBlockPtr add_block = block_chain_.at(hash);  // TODO(EJF): What happens when this fails

      // This won't re-call this function due to the flag
      InsertBlock(add_block, false);

      // The added block was not loose. Continue to clear
      auto it = loose_blocks_.find(add_block->body.hash);
      if (it != loose_blocks_.end())
      {
        // add all the items to the next list
        next_blocks_to_add.splice(next_blocks_to_add.end(), it->second);

        // remove the list from the blocks list
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
void MainChain::RecordLooseBlock(IntBlockPtr const &block)
{
  RLock lock(loose_mutex_);

  // Get vector of waiting blocks and push ours on
  auto &waiting_blocks = loose_blocks_[block->body.previous_hash];
  waiting_blocks.push_back(block->body.hash);

  block->is_loose                = true;
  block_chain_[block->body.hash] = block;
}

/**
 * Update tips (including the heaviest) if necessary
 *
 * @param block The current block
 * @param prev_block The previous block
 * @return true if the heaviest tip was advanced, otherwise false
 */
bool MainChain::UpdateTips(IntBlockPtr const &block)
{
  assert(!block->is_loose);
  assert(block->weight != 0);
  assert(block->total_weight != 0);

  // remove the tip if exists and add the new one
  tips_.erase(block->body.previous_hash);
  tips_[block->body.hash] = Tip{block->total_weight};

  // attempt to update the heaviest tip
  return heaviest_.Update(*block);
}

/**
 * Internal: Insert the block into the cache
 *
 * @param block The block be
 * @param evaluate_loose_blocks Flag to signal if the loose blocks should be evaluated
 * @return
 */
BlockStatus MainChain::InsertBlock(IntBlockPtr const &block, bool evaluate_loose_blocks)
{
  assert(block->body.previous_hash.size() > 0);

  fetch::generics::MilliTimer myTimer("MainChain::InsertBlock", 500);
  RLock                       lock(main_mutex_);

  if (block->body.hash.empty())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Block discard due to lack of digest");
    return BlockStatus::INVALID;
  }

  if (block->body.hash == block->body.previous_hash)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Block discard due to invalid digests");
    return BlockStatus::INVALID;
  }

  // Assume for the moment that this block is not loose. The validity of this statement will be
  // checked below
  block->is_loose = false;

  IntBlockPtr prev_block{};
  if (evaluate_loose_blocks)  // normal case - not being called from inside CompleteLooseBlocks
  {
    // First check if block already exists (not checking in object store)
    if (IsBlockInCache(block->body.hash))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Attempting to add already seen block");
      return BlockStatus::DUPLICATE;
    }

    // Determine if the block is present in the cache
    if (LookupBlock(block->body.previous_hash, prev_block))
    {
      // TODO(EJF): Add check to validate the block number (it is relied on heavily now)
      if (block->body.block_number != (prev_block->body.block_number + 1))
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Block ", ToBase64(block->body.hash),
                       " has invalid block number");
        return BlockStatus::INVALID;
      }

      // Also a loose block if it points to a loose block
      if (prev_block->is_loose)
      {
        // since we are connecting to loose block, by definition this block is also loose
        block->is_loose = true;

        FETCH_LOG_DEBUG(LOGGING_NAME, "Block connects to loose block");
      }
    }
    else
    {
      // This is the normal case where we do not have a previous hash
      block->is_loose = true;

      FETCH_LOG_DEBUG(LOGGING_NAME, "Previous block not found: ",
                      byte_array::ToBase64(block->body.previous_hash));
    }
  }
  else  // special case - being called from inside CompleteLooseBlocks
  {
    // This branch is a small optimisation since loose / missing blocks are not flushed to disk

    if (!LookupBlockFromCache(block->body.previous_hash, prev_block))
    {
      // This is currently only called from inside CompleteLooseBlocks and it is invariant on the
      // return value. For completeness this block is however loose because the parent can not be
      // located.
      return BlockStatus::LOOSE;
    }
  }

  if (block->is_loose)
  {
    // record the block as loose
    RecordLooseBlock(block);
    return BlockStatus::LOOSE;
  }

  // we exepect only non-loose blocks here
  assert(!block->is_loose);

  // by definition this also means we expect blocks to have a valid parent block too
  assert(static_cast<bool>(prev_block));

  // update the final (total) weight for this block
  block->total_weight = prev_block->total_weight + block->weight;

  // At this point we can proceed knowing that the block is building upon existing tip

  // At this point we have a new block with a prev that's known and not loose. Update tips
  bool const heaviest_advanced = UpdateTips(block);

  // Add block
  FETCH_LOG_DEBUG(LOGGING_NAME, "Adding block to chain: ", ToBase64(block->body.hash));
  AddBlockToCache(block);

  // If the heaviest branch has been updated we should determine if any blocks should be flushed
  // to disk
  if (heaviest_advanced)
  {
    WriteToFile();
  }

  // Now we're done, it's possible this added block completed some loose blocks.
  if (evaluate_loose_blocks)
  {
    CompleteLooseBlocks(block);
  }

  return BlockStatus::ADDED;
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
bool MainChain::LookupBlock(BlockHash hash, IntBlockPtr &block, bool add_to_cache) const
{
  return LookupBlockFromCache(hash, block) || LookupBlockFromStorage(hash, block, add_to_cache);
}

/**
 * Attempt to locate a block stored in the im memory cache
 *
 * @param hash The hash of the block to search for
 * @param block The output block to be populated
 * @return true if successful, otherwise false
 */
bool MainChain::LookupBlockFromCache(BlockHash hash, IntBlockPtr &block) const
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
bool MainChain::LookupBlockFromStorage(BlockHash hash, IntBlockPtr &block, bool add_to_cache) const
{
  bool success{false};

  if (block_store_)
  {
    // create the output block
    auto output_block = std::make_shared<Block>();

    // attempt to read the block from the storage engine
    success = block_store_->Get(storage::ResourceID(std::move(hash)), *output_block);

    if (success)
    {
      // hash not serialised, needs to be recomputed
      output_block->UpdateDigest();

      // add the newly loaded block to the cache (if required)
      if (add_to_cache)
      {
        AddBlockToCache(output_block);
      }

      // update the returned shared pointer
      block = output_block;
    }
  }

  return success;
}

/**
 * No Locking: Determine is a specified block is in the cache
 *
 * @param hash The hash to query
 * @return true if the block is present, otherwise false
 */
bool MainChain::IsBlockInCache(BlockHash hash) const
{
  return block_chain_.find(hash) != block_chain_.end();
}

/**
 * Add the block to the cache as a non-loose block
 *
 * @param block The block to be written into the cache
 */
void MainChain::AddBlockToCache(IntBlockPtr const &block) const
{
  block->is_loose = false;

  if (!IsBlockInCache(block->body.hash))
  {
    // add the item to the block chain storage
    block_chain_.emplace(block->body.hash, block);
  }
}

/**
 * Adds a new tip for the given block
 *
 * @param block The block for the tip to be created from
 */
bool MainChain::AddTip(IntBlockPtr const &block)
{
  RLock lock(main_mutex_);

  // record the tip weight
  tips_[block->body.hash] = Tip{block->total_weight};

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
          bool const is_heavier      = a.second.total_weight < b.second.total_weight;
          bool const is_equal_weight = a.second.total_weight == b.second.total_weight;
          bool const is_hash_larger  = a.first < b.first;

          return (is_heavier || (is_equal_weight && is_hash_larger));
        });

    // update the heaviest
    heaviest_.hash   = it->first;
    heaviest_.weight = it->second.total_weight;
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
  // TODO(private issue 666): Improve performance of block removal
  RLock lock(main_mutex_);

  // Step 1. Generate a new set of blocks (so that we can order them)
  std::vector<IntBlockPtr> block_list{};
  block_list.reserve(block_chain_.size());

  for (auto const &block : block_chain_)
  {
    block_list.push_back(block.second);
  }

  // Step 2. Order the blocks so that we have a good order for them to be evaluated
  std::sort(block_list.begin(), block_list.end(), [](IntBlockPtr const &a, IntBlockPtr const &b) {
    if (a->body.block_number == b->body.block_number)
    {
      return (a->body.hash < b->body.hash);
    }
    else
    {
      return (a->body.block_number < b->body.block_number);
    }
  });

  // Step 3. Loop through ordered blocks and generate the new list of tips
  BlockHashSet tips{};
  for (auto const &block : block_list)
  {
    tips.erase(block->body.previous_hash);

    // only non-loose blocks are considered tips
    if (!block->is_loose)
    {
      tips.insert(block->body.hash);
    }
  }
  block_list.clear();  // free some memory

  // Step 4. Recreate the tips map index
  TipsMap     new_tips{};
  IntBlockPtr block;
  for (auto const &tip : tips)
  {
    if (!LookupBlockFromCache(tip, block))
    {
      return false;
    }

    // add update the new tip
    new_tips[tip] = Tip{block->total_weight};
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
MainChain::IntBlockPtr MainChain::CreateGenesisBlock()
{
  auto genesis                = std::make_shared<Block>();
  genesis->body.previous_hash = GENESIS_DIGEST;
  genesis->body.merkle_hash   = GENESIS_MERKLE_ROOT;
  genesis->is_loose           = false;
  genesis->UpdateDigest();

  return genesis;
}

/**
 * Gets the current hash of the heaviest chain
 *
 * @return The heaviest chain hash
 */
MainChain::BlockHash MainChain::GetHeaviestBlockHash() const
{
  RLock lock(main_mutex_);
  return heaviest_.hash;
}

/**
 * Determines if the new block is the heaviest and if so updates its internal state
 *
 * @param block The candidate block being evaluated
 * @return true if the heaviest tip was updated, otherwise false
 */
bool MainChain::HeaviestTip::Update(Block const &block)
{
  bool updated{false};

  if ((block.total_weight > weight) || ((block.total_weight == weight) && (block.body.hash > hash)))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "New heaviest tip: ", ToBase64(block.body.hash));

    weight  = block.total_weight;
    hash    = block.body.hash;
    updated = true;
  }

  return updated;
}

}  // namespace ledger
}  // namespace fetch
