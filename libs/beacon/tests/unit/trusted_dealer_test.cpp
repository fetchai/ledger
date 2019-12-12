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

#include "beacon/beacon_service.hpp"
#include "beacon/create_new_certificate.hpp"
#include "beacon/trusted_dealer.hpp"
#include "beacon/trusted_dealer_beacon_service.hpp"
#include "core/reactor.hpp"
#include "muddle/muddle_interface.hpp"
#include "shards/manifest_cache_interface.hpp"

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

class DummyManifestCache : public fetch::shards::ManifestCacheInterface
{
public:
  DummyManifestCache()           = default;
  ~DummyManifestCache() override = default;

  bool QueryManifest(Address const & /*address*/, fetch::shards::Manifest & /*manifest*/) override
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
  TrustedDealerSetupService        setup_service;
  BeaconService                    beacon_service;
  crypto::Identity                 identity;

  TrustedDealerCabinetNode(uint16_t port_number, uint16_t index, double threshold,
                           uint64_t aeon_period)
    : event_manager{EventManager::New()}
    , muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{muddle::CreateMuddle("Test", muddle_certificate, network_manager, "127.0.0.1")}
    , setup_service{*muddle, manifest_cache, muddle_certificate, threshold, aeon_period}
    , beacon_service{*muddle, muddle_certificate, setup_service, event_manager}
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
                      double threshold = 0.5, uint64_t aeon_period = 10)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  std::cout << "- Setup" << std::endl;

  std::vector<std::unique_ptr<TrustedDealerCabinetNode>> cabinet;
  for (uint16_t ii = 0; ii < cabinet_size; ++ii)
  {
    auto port_number = static_cast<uint16_t>(10000 + ii);
    cabinet.emplace_back(new TrustedDealerCabinetNode{port_number, ii, threshold, aeon_period});
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
    member->reactor.Attach(member->beacon_service.GetWeakRunnable());
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
    uint64_t      start_time =
        GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM)) + 5;
    for (auto &member : cabinet)
    {
      member->setup_service.StartNewCabinet(cabinet_addresses, i * aeon_period, start_time,
                                            prev_entropy,
                                            dealer.GetDkgKeys(member->identity.identifier()));

      // Note, to avoid limiting the 'look ahead' entropy gen, set the block to ahead of numbers per
      // aeon
      member->beacon_service.MostRecentSeen((i * aeon_period) + aeon_period - 1);
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
        fetch::beacon::EventCabinetCompletedWork event;
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
  RunTrustedDealer(1, 4, 0.5, 10);
}
