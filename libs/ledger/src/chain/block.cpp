//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "core/serializers/main_serializer.hpp"
#include "crypto/merkle_tree.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chain/block.hpp"
#include "moment/clocks.hpp"

#include <cstddef>
#include <cstdint>

namespace fetch {
namespace ledger {

bool Block::operator==(Block const &rhs) const
{
  // Invalid to compare blocks with no block hash
  return !hash.empty() && hash == rhs.hash;
}

bool Block::operator!=(Block const &rhs) const
{
  return !operator==(rhs);
}

/**
 * Get the number of transactions present in the block
 *
 * @return The transaction count
 */
std::size_t Block::GetTransactionCount() const
{
  std::size_t count{0};

  for (auto const &slice : slices)
  {
    count += slice.size();
  }

  return count;
}

/**
 * Populate the block hash field based on the contents of the current block
 */
void Block::UpdateDigest()
{
  if (IsGenesis())
  {
    // genesis block's hash should be already set to a proper value and needs not to be updated
    assert(hash == chain::GetGenesisDigest());
    return;
  }

  crypto::MerkleTree tx_merkle_tree{GetTransactionCount()};

  // Populate the merkle tree
  std::size_t index{0};
  for (auto const &slice : slices)
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

  // clang-format off
  buf << previous_hash;
  buf << merkle_hash;
  buf << tx_merkle_tree.root();
  buf << block_number;
  buf << miner_id;
  buf << log2_num_lanes;
  buf << dag_epoch;
  buf << timestamp;
  buf << block_entropy;
  buf << weight;
  // clang-format on

  // Generate the hash
  crypto::SHA256 hash_builder;
  hash_builder.Reset();
  hash_builder.Update(buf.data());
  hash = hash_builder.Final();
}

void Block::UpdateTimestamp()
{
  if (!IsGenesis())
  {
    timestamp = GetTime(clock_);
  }
}

bool Block::IsGenesis() const
{
  return previous_hash == chain::ZERO_HASH;
}

}  // namespace ledger
}  // namespace fetch
