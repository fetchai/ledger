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
#include "vectorise/platform.hpp"

#include "block_generator.hpp"

#include <cstring>

BlockGenerator::BlockGenerator(std::size_t num_lanes, std::size_t num_slices)
  : num_slices_{num_slices}
{
  assert(fetch::platform::IsLog2(num_lanes));

  log2_num_lanes_ = fetch::platform::ToLog2(static_cast<uint32_t>(num_lanes));
}

void BlockGenerator::Reset()
{
  block_count_ = 0;
}

BlockGenerator::BlockPtr BlockGenerator::Generate(BlockPtr const &from, uint64_t weight)
{
  using fetch::byte_array::ByteArray;

  BlockPtr block = std::make_shared<Block>();

  // set the weight for this block
  block->weight = weight;

  if (from)
  {
    ByteArray ident{};
    ident.Resize(64);

    ByteArray merkle_root{};
    merkle_root.Resize(32);

    // zero pad the first half
    std::memset(merkle_root.pointer(), 0, 32 - sizeof(block_count_));

    // increment the block count
    ++block_count_;

    // make the block count the hash
    for (std::size_t i = 0; i < sizeof(block_count_); ++i)
    {
      std::size_t index = 32 - (i + 1);

      uint64_t const shift = i << 3;
      uint64_t const mask  = 0xFFllu << shift;

      // extract out the byte
      merkle_root[index] = static_cast<uint8_t>((block_count_ & mask) >> shift);
    }

    block->nonce               = ++block_count_;
    block->total_weight        = from->total_weight + block->weight;
    block->body.previous_hash  = from->body.hash;
    block->body.merkle_hash    = merkle_root;
    block->body.block_number   = from->body.block_number + 1u;
    block->body.miner          = ident;
    block->body.log2_num_lanes = log2_num_lanes_;
    block->body.slices.resize(num_slices_);
  }
  else
  {
    // update the previous hash
    block->body.previous_hash = fetch::ledger::GENESIS_DIGEST;
    block->body.merkle_hash   = fetch::ledger::GENESIS_MERKLE_ROOT;
  }

  // compute the digest for the block
  block->UpdateDigest();

  return block;
}

BlockGenerator::BlockPtr BlockGenerator::operator()(BlockPtr const &from, uint64_t weight)
{
  return Generate(from, weight);
}