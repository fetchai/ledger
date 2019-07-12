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
#include "dkg/beacon_manager.hpp"

#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>

using namespace fetch::crypto;
using namespace fetch::dkg;
using namespace fetch::byte_array;

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "usage: " << argv[0] << " [cabinet size]" << std::endl;
    return -1;
  }

  // Initializing the BLS library
  bls::Init();

  // Beacon parameters
  std::unordered_map<Identity, std::shared_ptr<BeaconManager>> nodes;
  uint64_t cabinet_size = static_cast<uint64_t>(atoi(argv[1]));
  uint32_t threshold    = static_cast<uint32_t>(cabinet_size >> 1);

  // Creating nodes
  for (uint64_t i = 0; i < cabinet_size; ++i)
  {
    std::shared_ptr<BeaconManager> node = std::make_shared<BeaconManager>();
    node->Reset(cabinet_size, threshold);
    nodes[node->identity()] = node;
  }

  // Communicating BLS identities
  using ParticipantDetails = std::pair<Identity, bls::Id>;
  std::vector<ParticipantDetails> participants;
  for (auto &m : nodes)
  {
    auto &n = *m.second;
    participants.push_back({n.identity(), n.id()});
  }

  // Propagating identities
  std::random_device rng_device;
  std::mt19937       generator(rng_device());
  for (auto &n : nodes)
  {
    // Shuffling to similate random arrival order
    std::shuffle(participants.begin(), participants.end(), generator);  

    for (auto &p : participants)
    {
      n.second->InsertMember(p.first, p.second);
    }
  }

  // Generating contributions
  for (auto &n : nodes)
  {
    n.second->GenerateContribution();
  }

  // Propagating shares & verification vectors
  for (auto &n : nodes)
  {
    // Get the verification from the sender node
    auto  from                = n.first;
    auto &sender              = *n.second;
    auto  verification_vector = sender.GetVerificationVector();

    // ... and promote it to all other nodes
    for (auto &m : nodes)
    {
      auto  to       = m.first;
      auto &receiver = *m.second;

      // ... along side the share.
      auto share = sender.GetShare(to);

      if (!receiver.AddShare(from, share, verification_vector))
      {
        throw std::runtime_error("share could not be verified.");
      }
    }
  }

  // Creating key public pairs
  for (auto &n : nodes)
  {
    n.second->CreateKeyPair();
  }

  // Setting next message for signing
  for (auto &n : nodes)
  {
    n.second->SetMessage("Hello world");
  }

  // Signing and broad casting message
  for (auto &n : nodes)
  {
    auto signed_message = n.second->Sign();
    auto identity       = n.second->identity();

    for (auto &m : nodes)
    {
      auto  to       = m.first;
      auto &receiver = *m.second;
      receiver.AddSignaturePart(identity, signed_message.public_key, signed_message.signature);
    }
  }

  // Verifying
  for (auto &n : nodes)
  {
    if (n.second->Verify())
    {
      std::cout << "Hurray, message verified." << std::endl;
    }
    else
    {
      throw std::runtime_error("Signature not verified");
    }
  }

  return 0;
}
