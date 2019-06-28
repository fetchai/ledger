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

#include "core/mutex.hpp"
#include "dkg/round.hpp"

namespace fetch {
namespace dkg {

void Round::AddShare(crypto::bls::Id const &id, crypto::bls::Signature const &sig)
{
  FETCH_LOCK(lock_);
  sig_ids_.push_back(id);
  sig_shares_.push_back(sig);
  ++num_shares_;
}

uint64_t Round::GetEntropy() const
{
  FETCH_LOCK(lock_);
  return *reinterpret_cast<uint64_t const *>(&round_signature_);
}

void Round::SetSignature(crypto::bls::Signature const &sig)
{
  FETCH_LOCK(lock_);
  round_signature_ = sig;
}

byte_array::ConstByteArray Round::GetRoundMessage() const
{
  FETCH_LOCK(lock_);
  auto const *raw = reinterpret_cast<uint8_t const *>(&round_signature_);
  return {raw, sizeof(crypto::bls::Signature)};
}

void Round::RecoverSignature()
{
  FETCH_LOCK(lock_);
  round_signature_ = crypto::bls::RecoverSignature(sig_shares_, sig_ids_);
  has_signature_   = true;
}

} // namespace dkg
} // namespace fetch
