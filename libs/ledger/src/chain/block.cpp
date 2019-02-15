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
#include "core/serializers/byte_array_buffer.hpp"
#include "crypto/sha256.hpp"

namespace fetch {
namespace ledger {

/**
 * Populate the block hash field based on the contents of the current block
 */
void Block::UpdateDigest()
{
  serializers::ByteArrayBuffer buf;
  buf << body.previous_hash << body.merkle_hash << body.block_number << nonce << body.miner;

  crypto::SHA256 hash;
  hash.Reset();
  hash.Update(buf.data());
  body.hash = hash.Final();

  proof.SetHeader(body.hash);
}

}  // namespace ledger
}  // namespace fetch
