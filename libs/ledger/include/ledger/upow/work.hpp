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

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/fnv.hpp"
#include "crypto/sha256.hpp"
#include "crypto/identity.hpp"
#include "ledger/upow/synergetic_base_types.hpp"
#include "ledger/chain/common_types.hpp"
#include "ledger/chain/address.hpp"
#include "math/bignumber.hpp"

#include <limits>
#include <memory>

namespace fetch {
namespace ledger {

class Work
{
public:
  enum
  {
    WORK_NOT_VALID = uint64_t(-1)
  };

  Work() = default;
  Work(Address address, crypto::Identity miner, BlockIndex block_number);
  Work(Work const &) = default;
  ~Work() = default;

  Address const &contract_digest() const;
  crypto::Identity const &miner() const;
//  BlockIndex block_number() const;
  math::BigUnsigned const &nonce() const;
  WorkScore score() const;

  void UpdateDigest(Address const &address)
  {
    contract_digest_ = address;
  }

  void UpdateIdentity(crypto::Identity const &identity)
  {
    miner_ = identity;
  }

  void UpdateScore(WorkScore score)
  {
    score_ = score;
  }

  void UpdateNonce(math::BigUnsigned const &nonce)
  {
    nonce_ = nonce;
  }

//  explicit operator bool() const;
  math::BigUnsigned CreateHashedNonce() const;

//  bool operator<(Work const &other) const;
//  bool operator==(Work const &other) const;
//  bool operator!=(Work const &other) const;

private:

  Address           contract_digest_{};
  crypto::Identity  miner_{};
//  BlockIndex        block_number_{WORK_NOT_VALID};
  math::BigUnsigned nonce_{};
  WorkScore         score_{std::numeric_limits<WorkScore>::max()};

  template <typename T>
  friend void Serialize(T &serializer, Work const &work);
  template <typename T>
  friend void Deserialize(T &serializer, Work &work);
};

inline Work::Work(Address address, crypto::Identity miner, BlockIndex block_number)
  : contract_digest_{std::move(address)}
  , miner_{std::move(miner)}
//  , block_number_{block_number}
{
  (void)block_number;
}

inline Address const &Work::contract_digest() const
{
  return contract_digest_;
}

inline crypto::Identity const &Work::miner() const
{
  return miner_;
}

//inline BlockIndex Work::block_number() const
//{
//  return block_number_;
//}

inline math::BigUnsigned const &Work::nonce() const
{
  return nonce_;
}

inline WorkScore Work::score() const
{
  return score_;
}

//inline Work::operator bool() const
//{
//  return block_number_ != WORK_NOT_VALID;
//}

/*
 * @brief computes the hashed nonce for work execution.
 */
inline math::BigUnsigned Work::CreateHashedNonce() const
{
  crypto::SHA256 hasher{};
  hasher.Reset();

  hasher.Update(contract_digest_.address());
  hasher.Update(miner_.identifier());
  hasher.Update(nonce_);

  auto const digest1 = hasher.Final();
  hasher.Reset();
  hasher.Update(digest1);

  auto const digest2 = hasher.Final();

  FETCH_LOG_INFO("Work", "---------------------------------------------------------------------------------------------");
  FETCH_LOG_INFO("Work", "Contract Digest: ", contract_digest_.address().ToHex());
  FETCH_LOG_INFO("Work", "Miner..........: ", miner_.identifier().ToHex());
  FETCH_LOG_INFO("Work", "Nonce..........: ", nonce_.ToHex());
  FETCH_LOG_INFO("Work", "Digest 1.......: ", digest1.ToHex());
  FETCH_LOG_INFO("Work", "Digest 2.......: ", digest2.ToHex());

  return digest2;
}

///*
// * @brief work can be prioritised based on
// */
//inline bool Work::operator<(Work const &other) const
//{
//  return score_ < other.score_;
//}
//
//inline bool Work::operator==(Work const &other) const
//{
//  return (block_number_ == other.block_number_) && (score_ == other.score_) &&
//         (nonce_ == other.nonce_) && (contract_digest_ == other.contract_digest_) &&
//         (miner_ == other.miner_);
//}
//
//inline bool Work::operator!=(Work const &other) const
//{
//  return !(*this == other);
//}

template <typename T>
void Serialize(T &serializer, Work const &work)
{
  serializer << /*work.block_number_ <<*/ work.nonce_ << work.score_;
}

template <typename T>
void Deserialize(T &serializer, Work &work)
{
  serializer >> /*work.block_number_ >>*/ work.nonce_ >> work.score_;
}

using WorkPtr = std::shared_ptr<Work>;

}  // namespace ledger
}  // namespace fetch
