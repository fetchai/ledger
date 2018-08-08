// TODO: This file is legacy

#pragma once

#include "assert.hpp"
#include "chain/transaction.hpp"
#include "crypto/fnv.hpp"
#include "protocols/chain_keeper/transaction_manager.hpp"

#include <map>
#include <stack>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace protocols {

class ChainManager
{
public:
  using hasher_type = crypto::CallableFNV;

  // Transaction defs
  using transaction_type = fetch::chain::Transaction;
  using tx_digest_type   = typename transaction_type::digest_type;

  // Block defs
  using proof_type        = fetch::chain::consensus::ProofOfWork;
  using block_body_type   = fetch::chain::BlockBody;
  using block_header_type = typename proof_type::header_type;
  using block_type        = fetch::chain::BasicBlock<proof_type, fetch::crypto::SHA256>;
  using shared_block_type = std::shared_ptr<block_type>;

  using chain_map_type = std::unordered_map<block_header_type, shared_block_type, hasher_type>;

  ChainManager() { group_ = 0; }

  enum
  {
    ADD_NOTHING_TODO = 0,
    ADD_CHAIN_END    = 2
  };

  bool AddBulkBlocks(std::vector<block_type> const &new_blocks)
  {
    bool ret = false;
    for (auto block : new_blocks)
    {
      ret |= (AddBlock(block) != ADD_NOTHING_TODO);
    }
    return ret;
  }

  uint32_t AddBlock(block_type &block)
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    //    fetch::logger.Debug("Entering ", __FUNCTION_NAME__);

    // Only record blocks that are new
    if (chains_.find(block.header()) != chains_.end())
    {
      //      fetch::logger.Debug("Nothing todo");
      return ADD_NOTHING_TODO;
    }

    TODO("Trim latest blocks");
    latest_blocks_.push_back(block);

    block_header_type header = block.header();

    chain_map_type::iterator pit;

    pit = chains_.find(block.body().previous_hash);

    if (pit != chains_.end())
    {
      block.set_previous(pit->second);
      block.set_is_loose(pit->second->is_loose());
    }
    else
    {
      // First block added is always genesis and by definition not loose
      block.set_is_loose(chains_.size() != 0);
    }

    auto shared_block       = std::make_shared<block_type>(block);
    chains_[block.header()] = shared_block;

    // TODO: Set next
    if (block.is_loose())
    {
      fetch::logger.Debug("Found loose block");
    }
    else if (!head_)
    {
      head_ = shared_block;
    }
    else if ((block.total_weight() >= head_->total_weight()))
    {
      head_ = shared_block;
    }

    return ADD_CHAIN_END;
  }

  shared_block_type const &head() const { return head_; }

  chain_map_type const &chains() const { return chains_; }

  chain_map_type &chains() { return chains_; }

  std::vector<block_type> const &latest_blocks() const { return latest_blocks_; }

  std::size_t size() const { return chains_.size(); }

  void set_group(uint32_t g) { group_ = g; }

private:
  std::atomic<uint32_t> group_;
  chain_map_type        chains_;

  shared_block_type head_;

  std::vector<block_type> latest_blocks_;
};
}  // namespace protocols
}  // namespace fetch
