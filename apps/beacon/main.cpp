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

#include "core/reactor.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "core/state_machine.hpp"
#include "crypto/bls_base.hpp"
#include "crypto/bls_dkg.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"

#include "network/generics/requesting_queue.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/muddle/subscription.hpp"

#include "beacon/beacon_service.hpp"
#include "beacon/beacon_setup_protocol.hpp"
#include "beacon/beacon_setup_service.hpp"
#include "beacon/cabinet_member_details.hpp"
#include "beacon/entropy.hpp"
#include "beacon/event_manager.hpp"

#include <cstdint>
#include <deque>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>
using namespace fetch;
using namespace fetch::beacon;

using Prover         = fetch::crypto::Prover;
using ProverPtr      = std::shared_ptr<Prover>;
using Certificate    = fetch::crypto::Prover;
using CertificatePtr = std::shared_ptr<Certificate>;
using Address        = fetch::muddle::Packet::Address;

ProverPtr CreateNewCertificate()
{
  using Signer    = fetch::crypto::ECDSASigner;
  using SignerPtr = std::shared_ptr<Signer>;

  SignerPtr certificate = std::make_shared<Signer>();

  certificate->GenerateKeys();

  return certificate;
}

struct CabinetNode
{
  using Prover         = crypto::Prover;
  using ProverPtr      = std::shared_ptr<Prover>;
  using Certificate    = crypto::Prover;
  using CertificatePtr = std::shared_ptr<Certificate>;
  using Muddle         = muddle::Muddle;

  uint16_t                muddle_port;
  network::NetworkManager network_manager;
  core::Reactor           reactor;
  ProverPtr               muddle_certificate;
  Muddle                  muddle;
  BeaconService           beacon_service;

  CabinetNode(uint16_t port_number, uint16_t index, EventManager::SharedEventManager event_manager)
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate, network_manager, true,
             true}
    , beacon_service{muddle.AsEndpoint(), muddle_certificate, event_manager}
  {
    network_manager.Start();
    muddle.Start({muddle_port});
  }
};

int main()
{
  constexpr uint16_t number_of_nodes    = 16;
  constexpr uint16_t cabinet_size       = 4;
  constexpr uint16_t number_of_cabinets = number_of_nodes / cabinet_size;

  // Initialising the BLS library
  crypto::bls::Init();

  EventManager::SharedEventManager event_manager = EventManager::New();

  std::vector<std::unique_ptr<CabinetNode>> committee;
  for (uint16_t ii = 0; ii < number_of_nodes; ++ii)
  {
    auto port_number = static_cast<uint16_t>(9000 + ii);
    committee.emplace_back(new CabinetNode{port_number, ii, event_manager});
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Connect muddles together (localhost for this example)
  for (uint32_t ii = 0; ii < number_of_nodes; ii++)
  {
    for (uint32_t jj = ii + 1; jj < number_of_nodes; jj++)
    {
      committee[ii]->muddle.AddPeer(
          fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(committee[jj]->muddle_port)});
    }
  }

  // Creating n cabinets
  BeaconService::CabinetMemberList all_cabinets[number_of_cabinets];

  uint64_t i = 0;
  for (auto &member : committee)
  {
    all_cabinets[i % number_of_cabinets].insert(member->muddle_certificate->identity());
    ++i;
  }

  // Attaching the cabinet logic
  for (auto &member : committee)
  {
    member->reactor.Attach(member->beacon_service.GetMainRunnable());
    member->reactor.Attach(member->beacon_service.GetSetupRunnable());
  }

  // Starting the beacon
  for (auto &member : committee)
  {
    member->reactor.Start();
  }

  // Ready
  uint64_t block_number = 0;
  uint64_t aeon_length  = 10;

  while (true)
  {
    if((block_number % aeon_length) == 0)
    {
      auto cabinet = all_cabinets[block_number % number_of_cabinets];
      for (auto &member : committee)
      {
        member->beacon_service.StartNewCabinet(cabinet, static_cast<uint32_t>(cabinet.size() / 2), block_number, block_number + aeon_length);
      }
    }

    uint64_t entropy;

    while(committee[block_number % committee.size()]->beacon_service.GenerateEntropy("", block_number, entropy) != fetch::ledger::EntropyGeneratorInterface::Status::OK)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    FETCH_LOG_INFO("default", "Found entropy for block: ", block_number, " as ", entropy);

    ++block_number;
  }

  return 0;
}
