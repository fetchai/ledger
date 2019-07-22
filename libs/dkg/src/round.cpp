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
#include "crypto/hash.hpp"
#include "crypto/mcl_dkg.hpp"
#include "crypto/sha256.hpp"
#include "dkg/round.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace fetch {
namespace dkg {

void Round::AddShare(uint32_t const &id, bn::G1 const &sig)
{
  FETCH_LOCK(lock_);

  // ensure no duplicates are present
  if (round_sig_shares_.find(id) == round_sig_shares_.end())
  {
    round_sig_shares_.insert({id, sig});
    ++num_shares_;
  }
}

uint64_t Round::GetEntropy() const
{
  FETCH_LOCK(lock_);
  return *reinterpret_cast<uint64_t const *>(round_entropy_.pointer());
}

void Round::SetSignature(bn::G1 const &sig)
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
  round_signature_ = LagrangeInterpolation(round_sig_shares_);
  has_signature_   = true;
  round_entropy_   = crypto::Hash<crypto::SHA256>(round_signature_.getStr());
}

}  // namespace dkg
}  // namespace fetch
