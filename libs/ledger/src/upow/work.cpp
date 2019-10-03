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
#include "core/digest.hpp"
#include "crypto/fnv.hpp"
#include "crypto/identity.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chain/common_types.hpp"
#include "ledger/upow/synergetic_base_types.hpp"
#include "ledger/upow/work.hpp"
#include "vectorise/uint/uint.hpp"

#include <limits>
#include <memory>
#include <vector>

namespace fetch {
namespace ledger {

Work::Work(Digest digest, Address address, crypto::Identity miner)
  : contract_digest_{std::move(digest)}
  , contract_address_{std::move(address)}
  , miner_{std::move(miner)}
{}

Digest const &Work::contract_digest() const
{
  return contract_digest_;
}

Address const &Work::address() const
{
  return contract_address_;
}

crypto::Identity const &Work::miner() const
{
  return miner_;
}

Work::UInt256 const &Work::nonce() const
{
  return nonce_;
}

WorkScore Work::score() const
{
  return score_;
}

void Work::UpdateDigest(Digest digest)
{
  contract_digest_ = std::move(digest);
}

void Work::UpdateAddress(Address address)
{
  contract_address_ = std::move(address);
}

void Work::UpdateIdentity(crypto::Identity const &identity)
{
  miner_ = identity;
}

void Work::UpdateScore(WorkScore score)
{
  score_ = score;
}

void Work::UpdateNonce(UInt256 const &nonce)
{
  nonce_ = nonce;
}

Work::UInt256 Work::CreateHashedNonce() const
{
  crypto::SHA256 hasher{};
  hasher.Reset();

  hasher.Update(contract_digest_);
  hasher.Update(miner_.identifier());
  hasher.Update(nonce_.pointer(), nonce_.size());

  auto const digest1 = hasher.Final();
  hasher.Reset();
  hasher.Update(digest1);

  return UInt256{hasher.Final()};
}

}  // namespace ledger
}  // namespace fetch
