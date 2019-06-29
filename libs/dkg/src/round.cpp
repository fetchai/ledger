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

#include "crypto/sha256.hpp"
#include "crypto/hash.hpp"
#include "core/mutex.hpp"
#include "dkg/round.hpp"

#include <algorithm>

static bool operator==(::blsId const &a, ::blsId const &b)
{
  bool equal{false};

  if (&a == &b)
  {
    equal = true;
  }
  else
  {
    equal = (0 == std::memcmp(&a, &b, sizeof(::blsId)));
  }

  return equal;
}

namespace fetch {
namespace dkg {

void Round::AddShare(crypto::bls::Id const &id, crypto::bls::Signature const &sig)
{
  FETCH_LOCK(lock_);

  // ensure no duplicates are present
  if (std::find(sig_ids_.begin(), sig_ids_.end(), id) == sig_ids_.end())
  {
    sig_ids_.push_back(id);
    sig_shares_.push_back(sig);
    ++num_shares_;
  }
}

uint64_t Round::GetEntropy() const
{
  FETCH_LOCK(lock_);
  return *reinterpret_cast<uint64_t const *>(round_entropy_.pointer());
}

void Round::SetSignature(crypto::bls::Signature const &sig)
{
  FETCH_LOCK(lock_);
  round_signature_ = sig;
}

byte_array::ConstByteArray Round::GetRoundEntropy() const
{
  FETCH_LOCK(lock_);
  return round_entropy_;
}

void Round::RecoverSignature()
{
  FETCH_LOCK(lock_);
  round_signature_ = crypto::bls::RecoverSignature(sig_shares_, sig_ids_);
  has_signature_   = true;
  round_entropy_   = crypto::Hash<crypto::SHA256>(crypto::bls::ToBinary(round_signature_));
}

} // namespace dkg
} // namespace fetch
