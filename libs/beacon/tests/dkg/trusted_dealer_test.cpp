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
#include "beacon/event_manager.hpp"
#include "beacon/trusted_dealer.hpp"
#include "core/reactor.hpp"
#include "core/service_ids.hpp"
#include "core/state_machine.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "ledger/shards/manifest.hpp"
#include "ledger/shards/manifest_cache_interface.hpp"
#include "muddle/muddle_interface.hpp"

#include "gtest/gtest.h"
#include <deque>
#include <iostream>

using namespace fetch;
using namespace fetch::beacon;
using namespace fetch::ledger;
using namespace std::chrono_literals;

using std::this_thread::sleep_for;
using std::chrono::milliseconds;

using Prover        = crypto::Prover;
using ProverPtr     = std::shared_ptr<Prover>;
using MuddleAddress = byte_array::ConstByteArray;

struct DummyManifestCache : public ManifestCacheInterface
{
  bool QueryManifest(Address const & /*address*/, Manifest & /*manifest*/) override
  {
    return false;
  }
};

class TrustedDealerBeaconService : public BeaconService
{
public:
  TrustedDealerBeaconService(MuddleInterface &               muddle,
                             ledger::ManifestCacheInterface &manifest_cache,
                             CertificatePtr                  certificate,
                             SharedEventManager              event_manager)  // NOLINT
    : BeaconService{muddle, manifest_cache, certificate, std::move(event_manager)} {};

  void StartNewCabinet(CabinetMemberList members, uint32_t threshold, uint64_t round_start,
                       uint64_t round_end, uint64_t start_time, const DkgOutput &output)
  {
    auto diff_time = int64_t(static_cast<uint64_t>(std::time(nullptr))) - int64_t(start_time);
    FETCH_LOG_INFO(LOGGING_NAME, "Starting new cabinet from ", round_start, " to ", round_end,
                   "at time: ", start_time, " (diff): ", diff_time);

    // Check threshold meets the requirements for the RBC
    uint32_t rbc_threshold{0};
    if (members.size() % 3 == 0)
    {
      rbc_threshold = static_cast<uint32_t>(members.size() / 3 - 1);
    }
    else
    {
      rbc_threshold = static_cast<uint32_t>(members.size() / 3);
    }
    if (threshold < rbc_threshold)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Threshold is below RBC threshold. Reset to rbc threshold");
      threshold = rbc_threshold;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    SharedAeonExecutionUnit beacon = std::make_shared<AeonExecutionUnit>();

    beacon->manager.SetCertificate(certificate_);
    beacon->manager.NewCabinet(members, threshold);
    beacon->manager.SetDkgOutput(output);

    // Setting the aeon details
    beacon->aeon.round_start               = round_start;
    beacon->aeon.round_end                 = round_end;
    beacon->aeon.members                   = std::move(members);
    beacon->aeon.start_reference_timepoint = start_time;

    // Even "observe only" details need to pass through the setup phase
    // to preserve order.
    aeon_exe_queue_.push_back(beacon);
  }
};

struct CabinetNode
{
  ProverPtr CreateNewCertificate()
  {
    using Signer    = fetch::crypto::ECDSASigner;
    using SignerPtr = std::shared_ptr<Signer>;

    SignerPtr certificate = std::make_shared<Signer>();

    certificate->GenerateKeys();

    return certificate;
  }

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

  CabinetNode(uint16_t port_number, uint16_t index)
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

  std::vector<std::unique_ptr<CabinetNode>> cabinet;
  for (uint16_t ii = 0; ii < cabinet_size; ++ii)
  {
    auto port_number = static_cast<uint16_t>(10000 + ii);
    cabinet.emplace_back(new CabinetNode{port_number, ii});
  }
  sleep_for(500ms);

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
    sleep_for(100ms);

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
          static_cast<uint64_t>(std::time(nullptr)), dealer.GetKeys(member->identity.identifier()));
    }

    // Wait for everyone to finish
    pending_nodes.clear();
    for (uint32_t ii = 0; ii < cabinet_size; ++ii)
    {
      pending_nodes.emplace(ii);
    }
    while (!pending_nodes.empty())
    {
      sleep_for(100ms);
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

TEST(beacon_service, DISABLED_trusted_dealer)
{
  bn::initPairing();
  RunTrustedDealer(3, 4, 3, 10);
}
