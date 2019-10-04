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

#include "beacon/create_new_certificate.hpp"
#include "core/reactor.hpp"
#include "ledger/shards/manifest_cache_interface.hpp"
#include "muddle/muddle_interface.hpp"

#include "beacon/trusted_dealer.hpp"
#include "beacon/trusted_dealer_beacon_service.hpp"

#include "gtest/gtest.h"

#include <iostream>

using namespace fetch;
using namespace fetch::muddle;
using namespace fetch::core;
using namespace fetch::crypto;
using namespace fetch::beacon;

using fetch::crypto::Prover;

using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using MuddleAddress = byte_array::ConstByteArray;

class DummyManifestCache : public fetch::ledger::ManifestCacheInterface
{
public:
  DummyManifestCache()           = default;
  ~DummyManifestCache() override = default;

  bool QueryManifest(Address const & /*address*/, fetch::ledger::Manifest & /*manifest*/) override
  {
    return true;
  }
};

struct TrustedDealerCabinetNode
{
  using Certificate    = crypto::Prover;
  using CertificatePtr = std::shared_ptr<Certificate>;
  using Muddle         = muddle::MuddlePtr;
  using Address        = fetch::muddle::Packet::Address;

  EventManager::SharedEventManager event_manager;
  uint16_t                         muddle_port;
  network::NetworkManager          network_manager;
  core::Reactor                    reactor;
  ProverPtr                        muddle_certificate;
  Muddle                           muddle;
  DummyManifestCache               manifest_cache;
  TrustedDealerBeaconService       beacon_service;
  crypto::Identity                 identity;

  TrustedDealerCabinetNode(uint16_t port_number, uint16_t index)
    : event_manager{EventManager::New()}
    , muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{muddle::CreateMuddle("Test", muddle_certificate, network_manager, "127.0.0.1")}
    , beacon_service{*muddle, manifest_cache, muddle_certificate, event_manager}
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

void RunTrustedDealer(uint16_t total_renewals = 4, uint32_t cabinet_size = 4,
                      uint32_t threshold = 3, uint16_t numbers_per_aeon = 10)
{
  std::cout << "- Setup" << std::endl;

  std::vector<std::unique_ptr<TrustedDealerCabinetNode>> cabinet;
  for (uint16_t ii = 0; ii < cabinet_size; ++ii)
  {
    auto port_number = static_cast<uint16_t>(10000 + ii);
    cabinet.emplace_back(new TrustedDealerCabinetNode{port_number, ii});
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Connect muddles together (localhost for this example)
  for (uint32_t ii = 0; ii < cabinet_size; ii++)
  {
    for (uint32_t jj = ii + 1; jj < cabinet_size; jj++)
    {
      cabinet[ii]->muddle->ConnectTo(cabinet[jj]->GetMuddleAddress(), cabinet[jj]->GetHint());
    }
  }

  // wait for all the nodes to completely connect
  std::unordered_set<uint32_t> pending_nodes;
  for (uint32_t ii = 0; ii < cabinet_size; ++ii)
  {
    pending_nodes.emplace(ii);
  }

  while (!pending_nodes.empty())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (auto it = pending_nodes.begin(); it != pending_nodes.end();)
    {
      auto &muddle = *(cabinet[*it]->muddle);

      if (static_cast<uint32_t>(muddle.GetNumDirectlyConnectedPeers() + 1) >= cabinet_size)
      {
        it = pending_nodes.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  std::set<MuddleAddress> cabinet_addresses;
  for (auto &member : cabinet)
  {
    cabinet_addresses.insert(member->muddle_certificate->identity().identifier());
  }

  // Attaching the cabinet logic
  for (auto &member : cabinet)
  {
    member->reactor.Attach(member->beacon_service.GetWeakRunnables());
  }

  // Starting the beacon
  for (auto &member : cabinet)
  {
    member->reactor.Start();
  }

  // Create previous entropy
  BlockEntropy prev_entropy;
  prev_entropy.group_signature = "Hello";

  // Ready
  uint64_t i = 0;
  while (i < total_renewals)
  {
    std::cout << "- Scheduling round " << i << std::endl;
    TrustedDealer dealer(cabinet_addresses, threshold);
    for (auto &member : cabinet)
    {
      member->beacon_service.StartNewCabinet(
          cabinet_addresses, threshold, i * numbers_per_aeon, (i + 1) * numbers_per_aeon,
          static_cast<uint64_t>(std::time(nullptr)), prev_entropy,
          dealer.GetKeys(member->identity.identifier()));
    }

    // Wait for everyone to finish
    pending_nodes.clear();
    for (uint32_t ii = 0; ii < cabinet_size; ++ii)
    {
      pending_nodes.emplace(ii);
    }
    while (!pending_nodes.empty())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      for (auto it = pending_nodes.begin(); it != pending_nodes.end();)
      {
        fetch::beacon::EventCommitteeCompletedWork event;
        if (cabinet[*it]->event_manager->Poll(event))
        {
          it = pending_nodes.erase(it);
        }
        else
        {
          ++it;
        }
      }
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
}

TEST(beacon_service, trusted_dealer)
{
  fetch::crypto::mcl::details::MCLInitialiser();
  RunTrustedDealer(1, 4, 3, 10);
}
