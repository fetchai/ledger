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

#include "core/serializers/main_serializer.hpp"
#include "crypto/merkle_tree.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/constants.hpp"
#include "moment/clocks.hpp"

#include <cstddef>
#include <cstdint>
#include <numeric>

namespace fetch {
namespace ledger {

Block::Block() = default;

bool Block::operator==(Block const &rhs) const
{
  // It is invalid to compare blocks with no block hash.
  return !body.hash.empty() && body.hash == rhs.body.hash;
}

/**
 * Get the number of transactions present in the block
 *
 * @return The transaction count
 */
std::size_t Block::GetTransactionCount() const
{
  return std::accumulate(body.slices.begin(), body.slices.end(), std::size_t{},
                         [](auto sum, auto &&slice) { return sum + slice.size(); });
}

/**
 * Populate the block hash field based on the contents of the current block
 */
void Block::UpdateDigest()
{
  crypto::MerkleTree tx_merkle_tree{GetTransactionCount()};

  // Populate the merkle tree
  std::size_t index{0};
  for (auto const &slice : body.slices)
  {
    for (auto const &tx : slice)
    {
      tx_merkle_tree[index++] = tx.digest();
    }
  }

  // Calculate the root
  tx_merkle_tree.CalculateRoot();

  // Generate hash stream
  serializers::MsgPackSerializer buf;
  buf << body.previous_hash << body.merkle_hash << body.block_number << body.miner
      << body.log2_num_lanes << body.timestamp << tx_merkle_tree.root() << nonce;

  // Generate the hash
  crypto::SHA256 hash;
  hash.Update(buf.data());
  body.hash = hash.Final();

  proof.SetHeader(body.hash);
}

void Block::UpdateTimestamp()
{
  if (!body.previous_hash.empty())
  {
    body.timestamp = GetTime(clock_);
  }
}

}  // namespace ledger
}  // namespace fetch
