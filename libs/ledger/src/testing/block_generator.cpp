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

#include "chain/constants.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/testing/block_generator.hpp"
#include "vectorise/platform.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

namespace fetch {
namespace ledger {
namespace testing {

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

BlockGenerator::BlockPtr BlockGenerator::Generate(BlockPtrConst const &from, uint64_t weight)
{
  using fetch::byte_array::ByteArray;

  auto block = std::make_shared<Block>();

  // set the weight for this block
  block->weight = weight;

  if (from)
  {
    ByteArray ident{};
    ident.Resize(32);

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

      uint64_t const shift = i << 3u;
      uint64_t const mask  = 0xFFllu << shift;

      // extract out the byte
      merkle_root[index] = static_cast<uint8_t>((block_count_ & mask) >> shift);
    }

    block->total_weight   = from->total_weight + block->weight;
    block->previous_hash  = from->hash;
    block->merkle_hash    = merkle_root;
    block->block_number   = from->block_number + 1u;
    block->miner          = chain::Address{ident};
    block->log2_num_lanes = log2_num_lanes_;
    block->slices.resize(num_slices_);

    block->UpdateTimestamp();

    // compute the digest for the block
    block->UpdateDigest();
  }
  else
  {
    block->previous_hash = chain::ZERO_HASH;
    block->hash          = chain::GENESIS_DIGEST;
    block->merkle_hash   = chain::GENESIS_MERKLE_ROOT;
    block->miner         = chain::Address{crypto::Hash<crypto::SHA256>("")};
    block->UpdateTimestamp();
  }

  return block;
}

BlockGenerator::BlockPtr BlockGenerator::operator()(BlockPtrConst const &from, uint64_t weight)
{
  return Generate(from, weight);
}

}  // namespace testing
}  // namespace ledger
}  // namespace fetch
