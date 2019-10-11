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

#include "beacon/block_entropy.hpp"
#include "meta/value_util.hpp"

using fetch::beacon::BlockEntropy;

BlockEntropy::BlockEntropy() = default;

/**
 * Copy constructor - no need for the digest or confirmations
 * since the copy constructor is used to create the next block
 * entropy
 */
BlockEntropy::BlockEntropy(BlockEntropy const &rhs)
  : qualified{rhs.qualified}
  , group_public_key{rhs.group_public_key}
  , block_number{rhs.block_number}
  , group_signature{rhs.group_signature}
{}

BlockEntropy &BlockEntropy::operator=(BlockEntropy const &that)
{
  qualified        = that.qualified;
  group_public_key = that.group_public_key;
  block_number     = that.block_number;
  group_signature  = that.group_signature;
  value_util::ClearAll(digest, confirmations);
  return *this;
}

BlockEntropy::Digest BlockEntropy::EntropyAsSHA256() const
{
  return crypto::Hash<crypto::SHA256>(group_signature);
}

// This will always be safe so long as the entropy function is properly sha256-ing
uint64_t BlockEntropy::EntropyAsU64() const
{
  const Digest hash = EntropyAsSHA256();
  return *reinterpret_cast<uint64_t const *>(hash.pointer());
}

void BlockEntropy::HashSelf()
{
  fetch::serializers::MsgPackSerializer serializer;
  serializer << qualified;
  serializer << group_public_key;
  serializer << block_number;
  digest = crypto::Hash<crypto::SHA256>(serializer.data());
}

bool BlockEntropy::IsAeonBeginning() const
{
  return !qualified.empty();
}
