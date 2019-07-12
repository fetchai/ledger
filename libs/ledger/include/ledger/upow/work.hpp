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
#include "crypto/identity.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chain/common_types.hpp"
#include "ledger/chain/digest.hpp"
#include "ledger/upow/synergetic_base_types.hpp"
#include "vectorise/uint/uint.hpp"

#include <limits>
#include <memory>
#include <vector>

namespace fetch {
namespace ledger {

class Work
{
public:
  using ConstByteArray = byte_array::ConstByteArray;
  using ProblemData    = std::vector<ConstByteArray>;
  using UInt256        = vectorise::UInt<256>;

  enum
  {
    WORK_NOT_VALID = uint64_t(-1)
  };

  // Construction / Destruction
  Work() = default;
  Work(Digest digest, crypto::Identity miner);
  Work(Work const &) = default;
  ~Work()            = default;

  // Getters
  Digest const &          contract_digest() const;
  crypto::Identity const &miner() const;
  UInt256 const &         nonce() const;
  WorkScore               score() const;

  // Setters
  void UpdateDigest(Digest digest);
  void UpdateIdentity(crypto::Identity const &identity);
  void UpdateScore(WorkScore score);
  void UpdateNonce(UInt256 const &nonce);

  // Actions
  UInt256 CreateHashedNonce() const;

private:
  Digest           contract_digest_{};
  crypto::Identity miner_{};
  UInt256          nonce_{};
  WorkScore        score_{std::numeric_limits<WorkScore>::max()};

  template <typename T>
  friend void Serialize(T &serializer, Work const &work);
  template <typename T>
  friend void Deserialize(T &serializer, Work &work);
};

inline Work::Work(Digest digest, crypto::Identity miner)
  : contract_digest_{std::move(digest)}
  , miner_{std::move(miner)}
{}

inline Digest const &Work::contract_digest() const
{
  return contract_digest_;
}

inline crypto::Identity const &Work::miner() const
{
  return miner_;
}

inline Work::UInt256 const &Work::nonce() const
{
  return nonce_;
}

inline WorkScore Work::score() const
{
  return score_;
}

inline void Work::UpdateDigest(Digest digest)
{
  contract_digest_ = std::move(digest);
}

inline void Work::UpdateIdentity(crypto::Identity const &identity)
{
  miner_ = identity;
}

inline void Work::UpdateScore(WorkScore score)
{
  score_ = score;
}

inline void Work::UpdateNonce(UInt256 const &nonce)
{
  nonce_ = nonce;
}

inline Work::UInt256 Work::CreateHashedNonce() const
{
  crypto::SHA256 hasher{};
  hasher.Reset();

  hasher.Update(contract_digest_);
  hasher.Update(miner_.identifier());
  hasher.Update(nonce_);

  auto const digest1 = hasher.Final();
  hasher.Reset();
  hasher.Update(digest1);

  return UInt256{hasher.Final()};
}

template <typename T>
void Serialize(T &serializer, Work const &work)
{
  serializer << work.nonce_ << work.score_;
}

template <typename T>
void Deserialize(T &serializer, Work &work)
{
  serializer >> work.nonce_ >> work.score_;
}

using WorkPtr = std::shared_ptr<Work>;

}  // namespace ledger
}  // namespace fetch
