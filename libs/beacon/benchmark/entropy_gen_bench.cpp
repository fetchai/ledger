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

#include "muddle/create_muddle_fake.hpp"

#include "beacon/trusted_dealer.hpp"
#include "beacon/trusted_dealer_beacon_service.hpp"

#include "benchmark/benchmark.h"

#include <type_traits>
#include <utility>
#include <vector>

using namespace fetch;
using namespace fetch::muddle;
using namespace fetch::core;
using namespace fetch::crypto;
using namespace fetch::beacon;

// Dummy manifest cache - does nothing but is required for beacon service constructor
class ManifestCacheInterfaceDummy : public fetch::ledger::ManifestCacheInterface
{
public:
  ManifestCacheInterfaceDummy()           = default;
  ~ManifestCacheInterfaceDummy() override = default;

  bool QueryManifest(Address const & /*address*/, fetch::ledger::Manifest & /*manifest*/) override
  {
    return true;
  }
};

struct BeaconSelfContained
{
  using CertificatePtr = std::shared_ptr<Prover>;
  using Muddle         = fetch::muddle::MuddlePtr;
  using ConstByteArray = byte_array::ConstByteArray;
  using MuddleAddress  = byte_array::ConstByteArray;
  using MessageType    = byte_array::ConstByteArray;

  ManifestCacheInterfaceDummy       dummy_manifest_cache;
  BeaconService::SharedEventManager event_manager = fetch::beacon::EventManager::New();

  BeaconSelfContained(uint16_t port_number, uint16_t index)
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 2}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{muddle::CreateMuddleFake("Test", muddle_certificate, network_manager, "127.0.0.1")}
    , beacon_service{*muddle.get(), dummy_manifest_cache, muddle_certificate, event_manager}
  {}

  void Start()
  {
    muddle->Start({muddle_port});
    reactor.Attach(beacon_service.GetWeakRunnables());
    reactor.Start();
  }

  virtual ~BeaconSelfContained()
  {
    reactor.Stop();
    muddle->Stop();
  }

  void ResetCabinet(BeaconService::CabinetMemberList /*unused*/ const &cabinet,
                    uint32_t round_start, uint32_t round_end, BlockEntropy const &entropy,
                    DkgOutput const &output)
  {
    beacon_service.StartNewCabinet(cabinet, static_cast<uint32_t>(cabinet.size() / 2 + 1),
                                   round_start, round_end,
                                   static_cast<uint64_t>(std::time(nullptr)), entropy, output);
  }

  muddle::Address GetMuddleAddress() const
  {
    return muddle->GetAddress();
  }

  network::Uri GetHint() const
  {
    return fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(muddle_port)};
  }

  uint16_t                   muddle_port;
  network::NetworkManager    network_manager;
  core::Reactor              reactor;
  ProverPtr                  muddle_certificate;
  Muddle                     muddle;
  std::mutex                 mutex;
  bool                       muddle_is_fake{true};
  TrustedDealerBeaconService beacon_service;
};

void EntropyGen(benchmark::State &state)
{
  SetGlobalLogLevel(LogLevel::ERROR);

  uint16_t unique_port    = 8000;
  uint16_t test_attempt   = 0;
  uint32_t entropy_rounds = 10;

  std::vector<std::unique_ptr<BeaconSelfContained>> nodes;
  BeaconService::CabinetMemberList                  cabinet;
  auto nodes_in_test = static_cast<uint64_t>(state.range(0));

  {
    nodes.clear();
    nodes.resize(nodes_in_test);

    for (uint16_t i = 0; i < nodes_in_test; ++i)
    {
      auto &node = nodes[i];
      if (!node)
      {
        node = std::make_unique<BeaconSelfContained>(unique_port + i, i);
        node->Start();
      }
      cabinet.insert(node->muddle_certificate->identity().identifier());
    }

    for (uint16_t i = 0; i < nodes_in_test; ++i)
    {
      auto &node = nodes[i];
      for (uint16_t j = 0; j < i; j++)
      {
        node->muddle->ConnectTo(nodes[j]->GetMuddleAddress(), nodes[j]->GetHint());
      }
    }

    // Wait until fully connected
    while (true)
    {
      bool connected = true;

      for (auto const &node : nodes)
      {
        if (node->muddle->GetNumDirectlyConnectedPeers() != nodes_in_test - 1)
        {
          connected = false;
        }
      }

      if (connected)
      {
        break;
      }
    }
  }

  for (auto _ : state)
  {
    state.PauseTiming();
    // Create previous entropy
    BlockEntropy prev_entropy;
    prev_entropy.group_signature = "Hello";
    TrustedDealer         dealer{cabinet, static_cast<uint32_t>(cabinet.size() / 2 + 1)};
    std::vector<uint32_t> pending_nodes(static_cast<uint32_t>(cabinet.size()));
    std::iota(pending_nodes.begin(), pending_nodes.end(), 0);
    for (auto const &node : nodes)
    {
      node->ResetCabinet(cabinet, test_attempt * entropy_rounds,
                         test_attempt * entropy_rounds + entropy_rounds, prev_entropy,
                         dealer.GetKeys(node->muddle_certificate->identity().identifier()));
    }
    state.ResumeTiming();

    // Wait for everyone to finish
    while (!pending_nodes.empty())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      for (auto it = pending_nodes.begin(); it != pending_nodes.end();)
      {
        fetch::beacon::EventCommitteeCompletedWork event;
        if (nodes[*it]->event_manager->Poll(event))
        {
          it = pending_nodes.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
    test_attempt++;
  }
}

// Threshold is 1/2 cabinet_size + 1
BENCHMARK(EntropyGen)->Range(4, 64)->Unit(benchmark::kMillisecond);
