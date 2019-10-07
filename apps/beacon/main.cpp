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
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "ledger/shards/manifest_cache_interface.hpp"

#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"
#include "network/generics/requesting_queue.hpp"

#include "beacon/beacon_service.hpp"
#include "beacon/beacon_setup_service.hpp"
#include "beacon/block_entropy.hpp"
#include "beacon/create_new_certificate.hpp"
#include "beacon/event_manager.hpp"

#include <cstdint>
#include <ctime>
#include <deque>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace fetch;
using namespace fetch::beacon;

using Prover         = fetch::crypto::Prover;
using ProverPtr      = std::shared_ptr<Prover>;
using Certificate    = fetch::crypto::Prover;
using CertificatePtr = std::shared_ptr<Certificate>;
using Address        = fetch::muddle::Packet::Address;

struct DummyManifestCache : public ledger::ManifestCacheInterface
{
  bool QueryManifest(Address const & /*address*/, ledger::Manifest & /*manifest*/) override
  {
    return false;
  }
};

struct CabinetNode
{
  using Prover         = crypto::Prover;
  using ProverPtr      = std::shared_ptr<Prover>;
  using Certificate    = crypto::Prover;
  using CertificatePtr = std::shared_ptr<Certificate>;
  using Muddle         = muddle::MuddlePtr;

  uint16_t                muddle_port;
  network::NetworkManager network_manager;
  core::Reactor           reactor;
  ProverPtr               muddle_certificate;
  Muddle                  muddle;
  DummyManifestCache      manifest_cache;
  BeaconService           beacon_service;

  CabinetNode(uint16_t port_number, uint16_t index, EventManager::SharedEventManager event_manager)
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{muddle::CreateMuddle("Test", muddle_certificate, network_manager, "127.0.0.1")}
    , beacon_service{*muddle, manifest_cache, muddle_certificate, std::move(event_manager)}
  {
    network_manager.Start();
    muddle->Start({muddle_port});
  }

  muddle::Address GetMuddleAddress() const
  {
    return muddle->GetAddress();
  }

  network::Uri GetHint() const
  {
    return fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(muddle_port)};
  }
};

int main()
{
  constexpr uint16_t number_of_nodes    = 16;
  constexpr uint16_t cabinet_size       = 4;
  constexpr uint16_t number_of_cabinets = number_of_nodes / cabinet_size;

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
      committee[ii]->muddle->ConnectTo(committee[jj]->GetMuddleAddress(), committee[jj]->GetHint());
    }
  }

  // Creating n cabinets
  BeaconService::CabinetMemberList all_cabinets[number_of_cabinets];

  uint64_t i = 0;
  for (auto &member : committee)
  {
    all_cabinets[i % number_of_cabinets].insert(
        member->muddle_certificate->identity().identifier());
    ++i;
  }

  // Attaching the cabinet logic
  for (auto &member : committee)
  {
    member->reactor.Attach(member->beacon_service.GetWeakRunnables());
  }

  // Starting the beacon
  for (auto &member : committee)
  {
    member->reactor.Start();
  }

  // Ready
  uint64_t     block_number = 0;
  uint64_t     aeon_length  = 10;
  BlockEntropy dummy_block_entropy;

  while (true)
  {
    if ((block_number % aeon_length) == 0)
    {
      auto cabinet = all_cabinets[block_number % number_of_cabinets];
      for (auto &member : committee)
      {
        member->beacon_service.StartNewCabinet(cabinet, static_cast<uint32_t>(cabinet.size() / 2),
                                               block_number, block_number + aeon_length,
                                               static_cast<uint64_t>(std::time(nullptr)),
                                               dummy_block_entropy);
      }
    }

    while (committee[block_number % committee.size()]->beacon_service.GenerateEntropy(
               block_number, dummy_block_entropy) !=
           fetch::ledger::EntropyGeneratorInterface::Status::OK)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    FETCH_LOG_INFO("default", "Found entropy for block: ", block_number, " as ",
                   dummy_block_entropy.EntropyAsU64());

    ++block_number;
  }

  return 0;
}
