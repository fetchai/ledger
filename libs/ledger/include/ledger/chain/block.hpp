#ifndef CHAIN_BLOCK_HPP
#define CHAIN_BLOCK_HPP
#include "core/byte_array/referenced_byte_array.hpp"
#include "ledger/chain/transaction.hpp"
#include "core/serializers/byte_array_buffer.hpp"

#include <memory>

namespace fetch {
namespace chain {

/*
struct BlockBody {
std::vector< fetch::byte_array::ByteArray > previous_hashes;
fetch::byte_array::ByteArray transaction_hash;
std::vector< uint32_t > groups;
};
*/

struct BlockBody {
  fetch::byte_array::ByteArray previous_hash;
  std::vector<TransactionSummary> transactions;
  uint32_t group_parameter;
};

template <typename T>
void Serialize(T &serializer, BlockBody const &body) {
  serializer << body.previous_hash << body.transactions << body.group_parameter;
}

template <typename T>
void Deserialize(T &serializer, BlockBody &body) {
  serializer >> body.previous_hash >> body.transactions >> body.group_parameter;
}

template <typename P, typename H>
class BasicBlock : public std::enable_shared_from_this<BasicBlock<P, H> > {
 public:
  typedef BlockBody body_type;
  typedef P proof_type;
  typedef H hasher_type;
  typedef typename proof_type::header_type header_type;
  typedef BasicBlock<P, H> self_type;
  typedef std::shared_ptr<self_type> shared_block_type;

  body_type const &SetBody(body_type const &body) {
    body_ = body;
    serializers::ByteArrayBuffer buf;

    buf << body;
    hasher_type hash;
    hash.Reset();
    hash.Update(buf.data());
    hash.Final();
    proof_.SetHeader(hash.digest());

    assert(hash.digest() == header());

    return body_;
  }

  header_type const &header() const { return proof_.header(); }
  proof_type const &proof() const { return proof_; }
  proof_type &proof() { return proof_; }
  body_type const &body() const { return body_; }

  void set_previous(shared_block_type const &p) { previous_ = p; }

  shared_block_type previous() const { return previous_; }

  shared_block_type shared_block() { return this->shared_from_this(); }

  double const &weight() const { return weight_; }

  double const &total_weight() const { return total_weight_; }

  double &weight() { return weight_; }

  void set_weight(double const &d) { weight_ = d; }

  double &total_weight() { return total_weight_; }

  void set_total_weight(double const &d) { total_weight_ = d; }

  void set_is_loose(bool b) { is_loose_ = b; }

  bool const &is_loose() const { return is_loose_; }

  bool const &is_verified() const { return is_verified_; }

  uint64_t block_number() const  // TODO Move to body
  {
    return block_number_;
  }

  void set_block_number(uint64_t const &b)  // TODO: Move to body
  {
    block_number_ = b;
  }

  uint64_t id() { return id_; }

  void set_id(uint64_t const &id) { id_ = id; }
  /*
  std::unordered_set< uint32_t > groups() {
    return groups_;
  }
  */
 private:
  body_type body_;
  proof_type proof_;

  // META data  and internal book keeping
  uint64_t block_number_;
  double weight_ = 0;
  double total_weight_ = 0;

  // Non-serialized meta-data
  //  std::unordered_map< uint32_t, shared_block_type > group_to_previous_;
  shared_block_type previous_;

  bool is_loose_ = true;
  bool is_verified_ = false;

  uint64_t id_ = uint64_t(-1);

  template <typename AT, typename AP, typename AH>
  friend inline void Serialize(AT &serializer, BasicBlock<AP, AH> const &);

  template <typename AT, typename AP, typename AH>
  friend inline void Deserialize(AT &serializer, BasicBlock<AP, AH> &b);
};

template <typename T, typename P, typename H>
inline void Serialize(T &serializer, BasicBlock<P, H> const &b) {
  serializer << b.body() << b.proof() << b.weight_
             << b.total_weight_;  // TODO: Fix should be computed
}

template <typename T, typename P, typename H>
inline void Deserialize(T &serializer, BasicBlock<P, H> &b) {
  BlockBody body;
  serializer >> body >> b.proof() >> b.weight_ >>
      b.total_weight_;  // TODO: FIX should be computed
  b.SetBody(body);
}
}
}
#endif
