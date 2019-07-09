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

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace fetch {
namespace crypto {
namespace bls {
namespace dkg {

using VerificationVector = std::vector<bls::PublicKey>;
using ContributionVector = std::vector<bls::PrivateKey>;
using ParticipantVector  = std::vector<bls::Id>;

struct Contribution
{
  VerificationVector verification;
  ContributionVector contributions;
};

Contribution GenerateContribution(ParticipantVector const &participants, uint32_t threshold)
{
  Contribution        ret;
  bls::PrivateKeyList private_keys;

  for (uint32_t i = 0; i < threshold; ++i)
  {
    auto sk = bls::PrivateKeyByCSPRNG();
    private_keys.push_back(sk);

    auto pk = bls::PublicKeyFromPrivate(sk);
    ret.verification.push_back(pk);
  }

  for (auto &id : participants)
  {
    auto sk = bls::PrivateKeyShare(private_keys, id);
    ret.contributions.push_back(sk);
  }

  return ret;
}

bls::PrivateKey AccumulateContributionShares(bls::PrivateKeyList list)
{
  if (list.empty())
  {
    throw std::runtime_error("Cannot accumulate empty list");
  }

  uint64_t i   = 0;
  auto     sum = list[i++];
  for (; i < list.size(); ++i)
  {
    bls::AddKeys(sum, list[i]);
  }

  return sum;
}

bool VerifyContributionShare(bls::Id id, bls::PrivateKey const &contribution,
                             bls::PublicKeyList const &pkl)
{
  bls::PublicKey pk1 = bls::PublicKeyShare(pkl, id);
  bls::PublicKey pk2 = bls::GetPublicKey(contribution);

  return bls::PublicKeyIsEqual(pk1, pk2);
}

VerificationVector AccumulateVerificationVectors(std::vector<VerificationVector> const &vectors)
{
  VerificationVector ret;
  auto               N = vectors.size();
  ret                  = vectors[0];
  for (uint64_t i = 1; i < N; ++i)
  {
    auto const &subvec = vectors[i];
    auto        M      = subvec.size();

    if (M == 0)
    {
      throw std::runtime_error("vector length is zero in AccumulateVerificationVectors.");
    }

    for (uint64_t j = 0; j < M; ++j)
    {
      bls::AddKeys(ret[j], subvec[j]);
    }
  }
  return ret;
}

}  // namespace dkg
}  // namespace bls
}  // namespace crypto
}  // namespace fetch
