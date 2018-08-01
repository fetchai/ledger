#pragma once
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
    hash.Final();
    body_.hash = hash.digest();

    proof_.SetHeader(body_.hash);
  }

  std::string summarise() const
  {
    char buffer[100];

    char *p = buffer;

    if (hash().size()>0)
    {
      for(size_t i=0;i<16;i++)
      {
        sprintf(p, "%02x", hash()[i]);
        p += 2;
      }
    }
    else
    {
      *p++ = '?';
    }

    *p++ = '-';
    *p++ = '>';

    if (body_.block_number == 0)
    {
      p += sprintf(p, "genesis");
    }
    else
    {
      if (body_.previous_hash.size()>0)
      {
        for(size_t i=0;i<16;i++)
        {
          sprintf(p, "%02x", body_.previous_hash[i]);
          p += 2;
        }
      }
      else
      {
        *p++ = '?';
        *p++ = '?';
        *p++ = '?';
      }
    }

    p += sprintf(p, " W=%d (%s)",
                 int(total_weight_),
                 is_loose_ ? "loose" : "attached"
                 );

    return std::string(buffer);
  }

  body_type const &  body() const { return body_; }
  body_type &        body() { return body_; }
  digest_type const &hash() const { return body_.hash; }
  digest_type const &prev() const { return body_.previous_hash; }

  proof_type const &proof() const { return proof_; }
  proof_type &      proof() { return proof_; }

  uint64_t &                    weight() { return weight_; }
  uint64_t &                    totalWeight() { return total_weight_; }
  bool &                        loose() { return is_loose_; }
  fetch::byte_array::ByteArray &root() { return root_; }

#if 1  // TODO: Move to py swarm?
  std::string hashString() const { return std::string(ToHex(body_.hash)); }

  std::string prevString() const { return std::string(ToHex(body_.previous_hash)); }
#endif

private:
  body_type  body_;
  proof_type proof_;

  // META data to help with block management
  uint64_t weight_       = 1;
  uint64_t total_weight_ = 1;
  bool     is_loose_     = false;

  // root refers to the previous_hash of the bottom block of a chain
  fetch::byte_array::ByteArray root_;

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
