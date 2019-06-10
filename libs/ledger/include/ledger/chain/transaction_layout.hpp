#pragma once
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

#include "core/bitvector.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/group_definitions.hpp"

namespace fetch {
namespace ledger {

class Transaction;

/**
 * A Transaction Layout is a summary class that extracts certain subset of information
 * from a transaction. This minimal set of information should be useful only for the mining/packing
 * of transactions into blocks.
 */
class TransactionLayout
{
public:
  using ConstByteArray = byte_array::ConstByteArray;
  using Digest         = byte_array::ConstByteArray;
  using TokenAmount    = uint64_t;
  using BlockIndex     = uint64_t;

  // Construction / Destruction
  TransactionLayout() = default;
  TransactionLayout(Transaction const &tx, uint32_t log2_num_lanes);
  TransactionLayout(Digest digest, BitVector const &mask, TokenAmount charge, BlockIndex valid_from,
                    BlockIndex valid_until);
  TransactionLayout(TransactionLayout const &) = default;
  TransactionLayout(TransactionLayout &&)      = default;
  ~TransactionLayout()                         = default;

  /// @name Accessors
  /// @{
  Digest const &   digest() const;
  BitVector const &mask() const;
  TokenAmount      charge() const;
  BlockIndex       valid_from() const;
  BlockIndex       valid_until() const;
  /// @}

  bool operator==(TransactionLayout const &other) const;

  // Operators
  TransactionLayout &operator=(TransactionLayout const &) = default;
  TransactionLayout &operator=(TransactionLayout &&) = default;

private:
  ConstByteArray digest_{};
  BitVector      mask_{};
  TokenAmount    charge_{0};
  BlockIndex     valid_from_{0};
  BlockIndex     valid_until_{0};

  // Native serializers
  template<typename T, typename D>
  friend struct serializers::MapSerializer;
};

/**
 * Get the associated transaction digest
 *
 * @return The transaction digest
 */
inline TransactionLayout::ConstByteArray const &TransactionLayout::digest() const
{
  return digest_;
}

/**
 * Get the shard mask usage for this transaction
 *
 * @return The shard mask
 */
inline BitVector const &TransactionLayout::mask() const
{
  return mask_;
}

/**
 * Get the charge (fee) associated with the transaction
 *
 * @return The charge amount
 */
inline TransactionLayout::TokenAmount TransactionLayout::charge() const
{
  return charge_;
}

/**
 * The block index from which point the transaction is valid
 *
 * @return The block index from when the block is valid
 */
inline TransactionLayout::BlockIndex TransactionLayout::valid_from() const
{
  return valid_from_;
}

/**
 * The block index until which the transaction is valid
 *
 * @return THe block index from which the transaction becomes invalid
 */
inline TransactionLayout::BlockIndex TransactionLayout::valid_until() const
{
  return valid_until_;
}

/**
 * Determine if the two objects are equal
 *
 * @param other THe other layout to compare against
 * @return true if equal, otherwise false
 */
inline bool TransactionLayout::operator==(TransactionLayout const &other) const
{
  return digest_ == other.digest_;
}

}  // namespace ledger
}  // namespace fetch

namespace std {

template <>
struct hash<fetch::ledger::TransactionLayout>
{
  std::size_t operator()(fetch::ledger::TransactionLayout const &layout) const
  {
    assert(layout.digest().size() >= sizeof(std::size_t));
    return *reinterpret_cast<std::size_t const *>(layout.digest().pointer());
  }
};

}  // namespace std
