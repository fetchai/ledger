//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "beacon/beacon_service.hpp"
#include "beacon/create_new_certificate.hpp"
#include "core/reactor.hpp"
#include "muddle/create_muddle_fake.hpp"
#include "muddle/muddle_interface.hpp"
#include "shards/manifest_cache_interface.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <ctime>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {

using namespace fetch;
using namespace fetch::beacon;
using namespace fetch::ledger;
using namespace std::chrono_literals;

using std::this_thread::sleep_for;
using std::chrono::milliseconds;

using Prover         = fetch::crypto::Prover;
using Certificate    = fetch::crypto::Prover;
using CertificatePtr = std::shared_ptr<Certificate>;
using Address        = fetch::muddle::Packet::Address;

struct DummyManifesttCache : public fetch::shards::ManifestCacheInterface
{
  bool QueryManifest(Address const & /*address*/, fetch::shards::Manifest & /*manifest*/) override
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

  EventManager::SharedEventManager event_manager;
  uint16_t                         muddle_port;
  network::NetworkManager          network_manager;
  core::Reactor                    reactor;
  ProverPtr                        muddle_certificate;
  Muddle                           muddle;
  DummyManifesttCache              manifest_cache;
  BeaconSetupService               beacon_setup_service;
  BeaconService                    beacon_service;
  crypto::Identity                 identity;
  BlockEntropy                     genesis_block_entropy;

  CabinetNode(uint16_t port_number, uint16_t index)
    : event_manager{EventManager::New()}
    , muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{muddle::CreateMuddleFake("Test", muddle_certificate, network_manager, "127.0.0.1")}
    , beacon_setup_service{*muddle, manifest_cache, muddle_certificate}
    , beacon_service{*muddle, muddle_certificate, beacon_setup_service, event_manager}
    , identity{muddle_certificate->identity()}
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

void RunHonestCabinetRenewal(uint16_t delay = 100, uint16_t total_renewals = 4,
                             uint16_t number_of_cabinets = 4, uint16_t cabinet_size = 4,
                             uint16_t numbers_per_aeon = 10, double threshold = 0.5)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  std::cout << "- Setup" << std::endl;
  auto number_of_nodes = static_cast<uint16_t>(number_of_cabinets * cabinet_size);

  std::vector<std::unique_ptr<CabinetNode>> cabinet;
  for (uint16_t ii = 0; ii < number_of_nodes; ++ii)
  {
    auto port_number = static_cast<uint16_t>(10000 + ii);
    cabinet.emplace_back(new CabinetNode{port_number, ii});
  }
  sleep_for(500ms);

  // Connect muddles together (localhost for this example)
  for (uint32_t ii = 0; ii < number_of_nodes; ii++)
  {
    for (uint32_t jj = ii + 1; jj < number_of_nodes; jj++)
    {
      cabinet[ii]->muddle->ConnectTo(cabinet[jj]->GetMuddleAddress(), cabinet[jj]->GetHint());
    }
  }

  // wait for all the nodes to completely connect
  std::unordered_set<uint32_t> pending_nodes;
  for (uint32_t ii = 0; ii < number_of_nodes; ++ii)
  {
    pending_nodes.emplace(ii);
  }

  const uint32_t EXPECTED_NUM_NODES = (number_of_cabinets * cabinet_size) - 1u;
  while (!pending_nodes.empty())
  {
    sleep_for(100ms);

    for (auto it = pending_nodes.begin(); it != pending_nodes.end();)
    {
      auto &muddle = *(cabinet[*it]->muddle);

      if (EXPECTED_NUM_NODES <= muddle.GetNumDirectlyConnectedPeers())
      {
        it = pending_nodes.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  // Creating n cabinets
  std::vector<BeaconSetupService::CabinetMemberList> all_cabinets;
  all_cabinets.resize(number_of_cabinets);

  uint64_t i = 0;
  for (auto &member : cabinet)
  {
    all_cabinets[i % number_of_cabinets].insert(
        member->muddle_certificate->identity().identifier());
    ++i;
  }

  // Attaching the cabinet logic
  for (auto &member : cabinet)
  {
    auto runnable = member->beacon_service.GetWeakRunnable();
    member->reactor.Attach(runnable);
  }

  // Starting the beacon
  for (auto &member : cabinet)
  {
    member->reactor.Start();
  }

  // Stats
  std::unordered_map<crypto::Identity, uint64_t> rounds_finished;
  for (auto &member : cabinet)
  {
    rounds_finished[member->identity] = 0;
  }

  // TODO(HUT): rewrite this test to check an unbroken stream of
  // entropy is generated
  // Ready
  i = 0;
  while (i < static_cast<uint64_t>(total_renewals + 1))
  {
    auto cabinet_select = all_cabinets[i % number_of_cabinets];

    if (i < total_renewals)
    {
      std::cout << "- Scheduling round " << i << std::endl;
      uint64_t start_time =
          GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM)) + 5;
      for (auto &member : cabinet)
      {
        member->beacon_setup_service.StartNewCabinet(
            cabinet_select,
            static_cast<uint32_t>(static_cast<double>(cabinet_select.size()) * threshold),
            i * numbers_per_aeon, (i + 1) * numbers_per_aeon, start_time,
            member->genesis_block_entropy);
      }
    }

    // Collecting information about the cabinets finishing
    for (uint64_t j = 0; j < 10; ++j)
    {
      for (auto &member : cabinet)
      {
        // Polling events about aeons completed work
        fetch::beacon::EventCabinetCompletedWork event;
        while (member->event_manager->Poll(event))
        {
          ++rounds_finished[member->identity];
        }
      }

      sleep_for(milliseconds{delay});
    }

    ++i;
  }

  std::cout << " - Stopping" << std::endl;
  for (auto &member : cabinet)
  {
    member->reactor.Stop();
    member->muddle->Stop();
    member->network_manager.Stop();
  }

  std::cout << " - Testing" << std::endl;
  // Verifying stats
  // TODO(tfr): Check that the hashes are acutally the same
  for (auto finish_stat : rounds_finished)
  {
    EXPECT_EQ(finish_stat.second, total_renewals);
  }
}

TEST(beacon, DISABLED_full_cycle)
{
  //  SetGlobalLogLevel(LogLevel::CRITICAL);
  // TODO(tfr): Heuristically fails atm. RunHonestCabinetRenewal(100, 4, 4, 4, 10, 0.5);
  RunHonestCabinetRenewal(100, 4, 2, 2, 10, 0.5);
}

}  // namespace
