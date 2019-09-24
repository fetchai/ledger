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

#include "core/byte_array/const_byte_array.hpp"
#include "core/reactor.hpp"
#include "crypto/ecdsa.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"
#include "ledger/shards/manifest_cache_interface.hpp"

#include "beacon/beacon_service.hpp"

#include "benchmark/benchmark.h"

#include <type_traits>
#include <vector>

using namespace fetch;
using namespace fetch::muddle;
using namespace fetch::core;
using namespace fetch::crypto;

using fetch::crypto::Prover;

ProverPtr CreateNewCertificate()
{
  using Signer    = fetch::crypto::ECDSASigner;
  using SignerPtr = std::shared_ptr<Signer>;

  SignerPtr certificate = std::make_shared<Signer>();

  certificate->GenerateKeys();

  return certificate;
}

class BeaconServiceInsertable : public fetch::beacon::BeaconService
{
public:

  BeaconServiceInsertable(MuddleInterface &muddle, ledger::ManifestCacheInterface &manifest_cache,
                CertificatePtr certificate, SharedEventManager event_manager,
                uint64_t blocks_per_round = 1) : BeaconService(muddle, manifest_cache, certificate, event_manager, blocks_per_round) {}

  void PushNewExecUnit(SharedAeonExecutionUnit beacon)
  {
    //jkjkstd::lock_guard<std::mutex> lock(mutex_);
    aeon_exe_queue_.push_back(beacon);
  }
};

// Dummy manifest cache - does nothing but is required for beacon service constructor
class ManifestCacheInterfaceDummy : public fetch::ledger::ManifestCacheInterface
{
public:

  ManifestCacheInterfaceDummy()          = default;
  ~ManifestCacheInterfaceDummy() = default;

  bool QueryManifest(Address const &, fetch::ledger::Manifest &) { return true; }
};

struct BeaconSelfContained
{
  using CertificatePtr = std::shared_ptr<Prover>;
  using Muddle         = fetch::muddle::MuddlePtr;
  using ConstByteArray = byte_array::ConstByteArray;
  using MuddleAddress  = byte_array::ConstByteArray;
  using MessageType    = byte_array::ConstByteArray;

  ManifestCacheInterfaceDummy dummy_manifest_cache;
  BeaconServiceInsertable::SharedEventManager event_manager = fetch::beacon::EventManager::New();

  BeaconSelfContained(uint16_t port_number, uint16_t index)
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 2}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{muddle::CreateMuddleFake("Test", muddle_certificate, network_manager, "127.0.0.1")}
    , beacon_service_insertable{*muddle.get(), dummy_manifest_cache, muddle_certificate, event_manager}
  {
  }

  void Start()
  {
    muddle->Start({muddle_port});
    reactor.Start();
  }

  virtual ~BeaconSelfContained()
  {
    reactor.Stop();
    muddle->Stop();
  }

  muddle::Address GetMuddleAddress() const
  {
    return muddle->GetAddress();
  }

  network::Uri GetHint() const
  {
    return fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(muddle_port)};
  }

  void ResetCabinet(BeaconServiceInsertable::CabinetMemberList const & /*cabinet*/)
  {
    // TODO(HUT): below code
    // Create static/global threshold signature
    //beacon_service_insertable.PushNewExecUnit(beacon);
  }

  uint16_t                             muddle_port;
  network::NetworkManager              network_manager;
  core::Reactor                        reactor;
  ProverPtr                            muddle_certificate;
  Muddle                               muddle;
  std::mutex                           mutex;
  bool                                 muddle_is_fake{true};
  BeaconServiceInsertable              beacon_service_insertable;
};

void EntropyGen(benchmark::State &state)
{
  char const *LOGGING_NAME    = "EntropyGen";

  SetGlobalLogLevel(LogLevel::ERROR);

  uint16_t unique_port  = 8000;
  uint16_t test_attempt = 0;

  std::vector<std::unique_ptr<BeaconSelfContained>> nodes;

  for (auto _ : state)
  {
    BeaconServiceInsertable::CabinetMemberList cabinet;

    uint64_t nodes_in_test = uint64_t(state.range(0));

    FETCH_LOG_INFO(LOGGING_NAME, "===============================");
    FETCH_LOG_INFO(LOGGING_NAME, "Starting test: ", nodes_in_test);

    // Setup here - not included in test timing
    {
      state.PauseTiming();
      nodes.clear();
      test_attempt++;

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

        for (uint16_t j = 0; j < i; j++)
        {
          node->muddle->ConnectTo(nodes[j]->GetMuddleAddress(), nodes[j]->GetHint());
        }
      }

      for (auto const &member : cabinet)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Cabinet member: ", member.ToBase64());
      }

      for (auto const &node : nodes)
      {
        node->ResetCabinet(cabinet);
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

      for (auto const &node : nodes)
      {
        node->Start();
      }

      state.ResumeTiming();
    }


    std::this_thread::sleep_for(std::chrono::milliseconds(100));

//    // Wait until all have seen all entropy
//    while (true)
//    {
//      bool all_seen = true;
//      uint64_t dummy_entropy;
//
//      for (auto const &node : nodes)
//      {
//        if (node->beacon_service_insertable.GenerateEntropy(ConstByteArray("none"), 99, dummy_entropy) != nodes_in_test - 1)
//        {
//          all_seen = false;
//        }
//      }
//
//      if (all_seen)
//      {
//        break;
//      }
//
//      std::this_thread::sleep_for(std::chrono::milliseconds(5));
//    }

    // SetGlobalLogLevel(LogLevel::INFO);
    FETCH_LOG_INFO(LOGGING_NAME, "Finished test: ", nodes_in_test);
    FETCH_LOG_INFO(LOGGING_NAME, "");
    // SetGlobalLogLevel(LogLevel::ERROR);
  }
}

BENCHMARK(EntropyGen)->Range(4, 64)->Unit(benchmark::kMillisecond);
