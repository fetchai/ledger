#pragma once
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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chain/transaction.hpp"
#include <memory>

namespace fetch {
namespace chain {

struct BlockSlice
{
  std::vector<TransactionSummary> transactions;
};

template <typename T>
void Serialize(T &serializer, BlockSlice const &slice)
{
  serializer << slice.transactions;
}

template <typename T>
void Deserialize(T &serializer, BlockSlice &slice)
{
  serializer >> slice.transactions;
}

struct BlockBody
{
  using digest_type       = byte_array::ConstByteArray;
  using block_slices_type = std::vector<BlockSlice>;

  digest_type hash;
  digest_type previous_hash;
  digest_type merkle_hash;
  // digest_type       state_hash;
  uint64_t          block_number{0};
  uint64_t          miner_number{0};
  uint64_t          nonce{0};
  uint32_t          log2_num_lanes{0};
  block_slices_type slices;
};

template <typename T>
void Serialize(T &serializer, BlockBody const &body)
{
  serializer << body.previous_hash << body.merkle_hash << body.nonce << body.block_number
             << body.miner_number << body.log2_num_lanes << body.slices;
}

template <typename T>
void Deserialize(T &serializer, BlockBody &body)
{
  serializer >> body.previous_hash >> body.merkle_hash >> body.nonce >> body.block_number >>
      body.miner_number >> body.log2_num_lanes >> body.slices;
}

template <typename P, typename H>
class BasicBlock
{
public:
  using body_type   = BlockBody;
  using hasher_type = H;
  using proof_type  = P;
  using digest_type = byte_array::ConstByteArray;

  body_type const &SetBody(body_type const &body)
  {
    body_ = body;
    return body_;
  }

  // Meta: Update hash
  void UpdateDigest()
  {

    serializers::ByteArrayBuffer buf;
    buf << body_.previous_hash << body_.merkle_hash << body_.block_number << body_.nonce
        << body_.miner_number;

    hasher_type hash;
    hash.Reset();
    hash.Update(buf.data());
    body_.hash = hash.Final();

    proof_.SetHeader(body_.hash);
  }


  body_type const &body() const
  {
    return body_;
  }
  body_type &body()
  {
    return body_;
  }
  digest_type const &hash() const
  {
    return body_.hash;
  }
  digest_type const &prev() const
  {
    return body_.previous_hash;
  }

  proof_type const &proof() const
  {
    return proof_;
  }
  proof_type &proof()
  {
    return proof_;
  }

  uint64_t &weight()
  {
    return weight_;
  }
  uint64_t &totalWeight()
  {
    return total_weight_;
  }
  uint64_t const &totalWeight() const
  {
    return total_weight_;
  }
  bool &loose()
  {
    return is_loose_;
  }

#if 1  // TODO(issue 33): Move to py swarm?
  std::string hashString() const
  {
    return std::string(ToBase64(body_.hash));
  }

  std::string prevString() const
  {
    return std::string(ToBase64(body_.previous_hash));
  }
#endif

private:
  body_type  body_;
  proof_type proof_;

  // META data to help with block management
  uint64_t weight_       = 1;
  uint64_t total_weight_ = 1;
  bool     is_loose_     = true;

  template <typename AT, typename AP, typename AH>
  friend inline void Serialize(AT &serializer, BasicBlock<AP, AH> const &);

  template <typename AT, typename AP, typename AH>
  friend inline void Deserialize(AT &serializer, BasicBlock<AP, AH> &b);
};

template <typename T, typename P, typename H>
inline void Serialize(T &serializer, BasicBlock<P, H> const &b)
{
  serializer << b.body_ << b.proof();
}

template <typename T, typename P, typename H>
inline void Deserialize(T &serializer, BasicBlock<P, H> &b)
{
  BlockBody body;
  serializer >> body >> b.proof();
  b.SetBody(body);
}
}  // namespace chain
}  // namespace fetch
