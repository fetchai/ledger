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

#include "ledger/chain/block.hpp"
#include "ledger/chain/constants.hpp"

#include <memory>

class BlockGenerator
{
public:
  using Block    = fetch::ledger::Block;
  using BlockPtr = std::shared_ptr<Block>;

  void Reset()
  {
    block_count_ = 0;
  }

  BlockPtr operator()(BlockPtr const &from = BlockPtr{}, uint64_t weight = 1u)
  {
    BlockPtr block = std::make_shared<Block>();

    // set the weight for this block
    block->weight = weight;

    if (from)
    {
      block->nonce              = ++block_count_;
      block->total_weight       = from->total_weight + block->weight;
      block->body.previous_hash = from->body.hash;
      block->body.block_number  = from->body.block_number + 1u;
    }
    else
    {
      // update the previous hash
      block->body.previous_hash = fetch::chain::GENESIS_DIGEST;
    }

    // compute the digest for the block
    block->UpdateDigest();

    return block;
  }

private:
  uint64_t block_count_{0};
};
