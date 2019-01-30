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

#include <utility>

namespace fetch {
namespace ledger {

MainChain::MainChain(bool disable_persistence)
{
  Block genesis                   = CreateGenesisBlock();
  block_chain_[genesis.body.hash] = genesis;

  // Create tip for genesis
  auto tip                 = std::make_shared<Tip>();
  tip->total_weight        = genesis.weight;
  tips_[genesis.body.hash] = tip;
  heaviest_                = std::make_pair(genesis.weight, genesis.body.hash);

  if (!disable_persistence)
  {
    // create the block store
    block_store_ = std::make_unique<BlockStore>();

    RecoverFromFile();
  }
}

bool MainChain::AddBlock(Block &block, bool recursive_iteration)
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

  if (!recursive_iteration)
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
      NewLooseBlock(block);
      return true;
    }
  }
  else  // else of recursive_iteration==true
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
  if (!recursive_iteration)
  {
    CompleteLooseBlocks(block);
  }

  return true;
}

Block const &MainChain::HeaviestBlock() const
{
  RLock lock(main_mutex_);

  auto const &block = block_chain_.at(heaviest_.second);

  return block;
}

MainChain::Blocks MainChain::HeaviestChain(uint64_t const &limit) const
{
  fetch::generics::MilliTimer myTimer("MainChain::HeaviestChain");
  RLock                       lock(main_mutex_);

  Blocks result;

  auto topBlock = block_chain_.at(heaviest_.second);

  std::size_t additional_blocks = 0;
  if (limit > 1)
  {
    additional_blocks = static_cast<std::size_t>(limit - 1);
  }

  while ((topBlock.body.block_number != 0) && (result.size() < additional_blocks))
  {
    result.push_back(topBlock);
    auto hash = topBlock.body.previous_hash;

    // Walk down
    auto it = block_chain_.find(hash);
    if (it == block_chain_.end())
    {
      FETCH_LOG_INFO(LOGGING_NAME,
                     "Mainchain: Failed while walking down from top block to find genesis!");
      break;
    }

    topBlock = it->second;
  }

  result.push_back(topBlock);  // this should be genesis
  return result;
}

MainChain::Blocks MainChain::ChainPreceding(BlockHash const &at, uint64_t limit) const
{
  fetch::generics::MilliTimer myTimer("MainChain::ChainPreceding");
  RLock                       lock(main_mutex_);

  Blocks result;

  auto topBlock = block_chain_.at(at);

  while (result.size() < limit)
  {
    result.push_back(topBlock);
    if (topBlock.body.block_number == 0)
    {
      break;
    }
    auto hash = topBlock.body.previous_hash;
    // Walk down
    auto it = block_chain_.find(hash);
    if (it == block_chain_.end())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Mainchain: Failed while walking down from ",
                     byte_array::ToBase64(at), " to find genesis!");
      break;
    }
    topBlock = it->second;
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
bool MainChain::GetCommonSubTree(Blocks &blocks, BlockHash tip, BlockHash node, uint64_t limit)
{
  fetch::generics::MilliTimer myTimer("MainChain::GetCommonSubTree");
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
        break;
      }
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Left: ", ToBase64(left_hash), " -> ", left.total_weight,
                    " Right: ", ToBase64(right_hash), " -> ", right.total_weight);

    if (left_hash == right_hash)
    {
      // exit condition
      success = true;
      break;
    }
    else
    {
      if (left.total_weight <= right.total_weight)
      {
        right_hash = right.body.previous_hash;
      }

      if (left.total_weight >= right.total_weight)
      {
        left_hash = left.body.previous_hash;
      }
    }
  }

  return success;
}

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
  heaviest_                = std::make_pair(genesis.weight, genesis.body.hash);
}

bool MainChain::Get(BlockHash hash, Block &block)
{
  RLock lock(main_mutex_);

  bool success = false;

  // attempt to lookup the hash in the block maps
  auto it = block_chain_.find(hash);
  if (it != block_chain_.end())
  {
    block   = it->second;
    success = true;
  }
  else if (block_store_)
  {
    // attempt to lookup the block from the store
    success = block_store_->Get(storage::ResourceID(hash), block);

    // All blocks in block store are guaranteed not loose
    if (success)
    {
      block.is_loose = false;
    }
  }

  return success;
}

MainChain::BlockHashs MainChain::GetMissingBlockHashes(std::size_t maximum)
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

void MainChain::RecoverFromFile()
{
  assert(static_cast<bool>(block_store_));

  block_store_->Load("chain.db", "chain.index.db");

  Block block;
  if (block_store_->Get(storage::ResourceAddress("head"), block))
  {
    block.UpdateDigest();
    AddBlock(block);

    while (block_store_->Get(storage::ResourceID(block.body.previous_hash), block))
    {
      block.UpdateDigest();
      AddBlock(block);
    }
  }
}

void MainChain::WriteToFile()
{
  // skip if the block store is not persistent
  if (!block_store_)
    return;

  fetch::generics::MilliTimer myTimer("MainChain::WriteToFile");

  if (block_store_)
  {
    // Add confirmed blocks to file
    Block block  = block_chain_.at(heaviest_.second);
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
      AddBlock(add_block, true);

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

void MainChain::NewLooseBlock(Block &block)
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
    NewLooseBlock(block);
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
    NewLooseBlock(block);
    return false;  ///? should this be true true here?
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

  return AddBlock(block);
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
    if ((tip->total_weight > heaviest_.first) ||
        ((tip->total_weight == heaviest_.first) && (block.body.hash > heaviest_.second)))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Updating heaviest with tip");

      heaviest_.first  = tip->total_weight;
      heaviest_.second = block.body.hash;

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
    if ((tip->total_weight > heaviest_.first) ||
        ((tip->total_weight == heaviest_.first) && (block.body.hash > heaviest_.second)))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "creating new tip that is now heaviest! (new fork)");

      heaviest_.first   = tip->total_weight;
      heaviest_.second  = block.body.hash;
      heaviest_advanced = true;
    }
  }

  if (tip)
  {
    tips_[block.body.hash] = tip;
  }

  return heaviest_advanced;
}

Block MainChain::CreateGenesisBlock()
{
  Block genesis{};
  genesis.body.previous_hash = chain::GENESIS_DIGEST;
  genesis.is_loose           = false;
  genesis.UpdateDigest();

  return genesis;
}

}  // namespace ledger
}  // namespace fetch
