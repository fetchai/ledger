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

#include "core/macros.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "ledger/consensus/naive_entropy_generator.hpp"

namespace fetch {
namespace ledger {

using Status = NaiveEntropyGenerator::Status;

/**
 * Generate Entropy for a specified block period
 *
 * @param block_digest The specified block digest for the period
 * @param block_number The block number
 * @return The generated entropy
 */
Status NaiveEntropyGenerator::GenerateEntropy(Digest block_digest, uint64_t block_number,
                                              uint64_t &entropy)
{
  FETCH_UNUSED(block_number);

  // perform repeated hashes of the block digest
  for (std::size_t i = 0; i < ROUNDS; ++i)
  {
    block_digest = crypto::Hash<crypto::SHA256>(block_digest);
  }

  auto const &digest_ref = block_digest;
  entropy = *reinterpret_cast<uint64_t const *>(digest_ref.pointer());

  return Status::OK;
}

}  // namespace ledger
}  // namespace fetch
