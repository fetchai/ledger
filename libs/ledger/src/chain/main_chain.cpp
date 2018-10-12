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

#include "ledger/chain/main_chain.hpp"


namespace fetch {
namespace chain {

bool MainChain::AddBlock(BlockType &block, bool recursive_iteration)
{
  assert(block.body().previous_hash.size() > 0);

  fetch::generics::MilliTimer           myTimer("MainChain::AddBlock");
  std::unique_lock<Mutex> lock(main_mutex_); // we need to selectively unlock this later.

  if (block.hash().size() == 0)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Erk! You called AddBlock with no UpdateDigest");
  }

  BlockType prev_block;

  if (!recursive_iteration)
  {
    // First check if block already exists (not checking in object store)
    if (block_chain_.find(block.hash()) != block_chain_.end())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Attempting to add already seen block");
      return false;
    }

    // Look for the prev block to this one
    auto it_prev = block_chain_.find(block.body().previous_hash);

    if (it_prev == block_chain_.end())
    {
      // Note: think about rolling back into FS
      FETCH_LOG_INFO(LOGGING_NAME,
                     "Block prev not found: ", byte_array::ToBase64(block.body().previous_hash));

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
    auto it = block_chain_.find(block.body().previous_hash);
    assert(it != block_chain_.end());

    prev_block = it->second;
  }

  // At this point we have a new block with a prev that's known and not loose. Update tips
  block.loose()         = false;
  bool heaviestAdvanced = UpdateTips(block, prev_block);

  // Add block
  FETCH_LOG_DEBUG(LOGGING_NAME, "Adding block to chain: ", ToBase64(block.hash()));
  block_chain_[block.hash()] = block;

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

}
}
