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

#include "crypto/bls_base.hpp"

#include <memory>
#include <mutex>

namespace fetch {
namespace dkg {

class Round
{
public:
  using ConstByteArray = byte_array::ConstByteArray;

  // Construction / Destruction
  explicit Round(uint64_t round);
  Round(Round const &) = delete;
  Round(Round &&)      = delete;
  ~Round()             = default;

  uint64_t                      round() const;
  crypto::bls::Signature const &round_signature() const
  {
    return round_signature_;
  }

  void           AddShare(crypto::bls::Id const &id, crypto::bls::Signature const &sig);
  bool           HasSignature() const;
  std::size_t    GetNumShares() const;
  uint64_t       GetEntropy() const;
  void           SetSignature(crypto::bls::Signature const &sig);
  ConstByteArray GetRoundMessage() const;
  void           RecoverSignature();

  // Operators
  Round &operator=(Round const &) = delete;
  Round &operator=(Round &&) = delete;

private:
  uint64_t const             round_;
  mutable std::mutex         lock_{};
  crypto::bls::IdList        sig_ids_{};
  crypto::bls::SignatureList sig_shares_{};
  crypto::bls::Signature     round_signature_{};
  std::atomic<std::size_t>   num_shares_{0};
  std::atomic<bool>          has_signature_{};
};

inline Round::Round(uint64_t round)
  : round_{round}
{}

inline uint64_t Round::round() const
{
  return round_;
}

inline bool Round::HasSignature() const
{
  return has_signature_;
}

inline std::size_t Round::GetNumShares() const
{
  return num_shares_;
}

using RoundPtr = std::shared_ptr<Round>;

}  // namespace dkg
}  // namespace fetch
