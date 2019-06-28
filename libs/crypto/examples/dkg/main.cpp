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
#include "crypto/bls_dkg.hpp"
#include <iostream>

using namespace fetch::crypto;
using namespace fetch::byte_array;

struct Member
{
  ConstByteArray      seed;
  bls::Id             id;
  bls::PrivateKey     sk;
  bls::PrivateKeyList received_shares;
  bls::PrivateKey     secret_key_share;
};

void Test()
{
  ConstByteArray message     = "Hello world";
  auto           private_key = bls::HashToPrivateKey("my really long phrase to generate a key");
  auto           public_key  = bls::PublicKeyFromPrivate(private_key);

  auto signature = bls::Sign(private_key, message);
  if (bls::Verify(signature, public_key, message))
  {
    std::cout << "'Hello world' was signed." << std::endl;
  }
}

int main()
{
  using VerificationVector = bls::dkg::VerificationVector;
  using ParticipantVector  = bls::dkg::ParticipantVector;

  bls::Init();

  // Creating members from predefined seeds
  std::vector<ConstByteArray> member_seeds = {"12122",    "454323", "547456", "54",
                                              "23423423", "68565",  "56465"};
  std::vector<Member>         members;
  uint32_t                    threshold = 4;

  for (auto &seed : member_seeds)
  {
    Member member;
    member.seed = seed;
    member.sk   = bls::PrivateKeyByCSPRNG();
    members.push_back(member);
  }

  // Creating participant list
  ParticipantVector participants;
  for (auto &member : members)
  {
    member.id.v = member.sk.v;
    participants.push_back(member.id);
  }

  // Generating secret and collecting secrets.
  std::vector<VerificationVector> verification_vectors;
  for (uint64_t i = 0; i < members.size(); ++i)
  {
    auto contrib = bls::dkg::GenerateContribution(participants, threshold);
    // Note that the verfication vector can be posted publicly.
    verification_vectors.push_back(contrib.verification);

    for (uint64_t j = 0; j < contrib.contributions.size(); ++j)
    {
      auto  spk      = contrib.contributions[j];
      auto &member   = members[j];
      bool  verified = bls::dkg::VerifyContributionShare(member.id, spk, contrib.verification);

      if (!verified)
      {
        throw std::runtime_error("share could not be verified.");
      }

      member.received_shares.push_back(spk);
    }
  }

  // Generating secret key share
  for (auto &member : members)
  {
    member.secret_key_share = bls::dkg::AccumulateContributionShares(member.received_shares);
  }

  // We now use the publicly disclosed verification vectors to generate group vectors.
  VerificationVector group_vectors = bls::dkg::AccumulateVerificationVectors(verification_vectors);

  // The groups public key is the first vector.
  auto groups_pk = group_vectors[0];

  // We now sign the message
  ConstByteArray     message = "Hello world";
  bls::SignatureList signatures;
  bls::IdList        signer_ids;

  for (uint64_t i = 0; i < threshold; ++i)
  {
    auto private_key = members[i].secret_key_share;
    auto signature   = bls::Sign(private_key, message);
    auto public_key  = bls::PublicKeyFromPrivate(private_key);

    if (!bls::Verify(signature, public_key, message))
    {
      throw std::runtime_error("Failed to sign using share");
    }

    signatures.push_back(signature);
    signer_ids.push_back(members[i].id);
  }

  // And finally we test the signature
  auto signature = bls::RecoverSignature(signatures, signer_ids);
  if (bls::Verify(signature, groups_pk, message))
  {
    std::cout << " -> Hurray, the signature is valid!" << std::endl;
  }
  else
  {
    throw std::runtime_error("Signature not working!!");
  }

  return 0;
}
