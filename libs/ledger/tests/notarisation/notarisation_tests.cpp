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
#include "beacon/trusted_dealer_beacon_service.hpp"
#include "core/reactor.hpp"
#include "crypto/mcl_dkg.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/consensus/consensus.hpp"
#include "ledger/consensus/stake_snapshot.hpp"
#include "ledger/protocols/notarisation_service.hpp"
#include "ledger/storage_unit/fake_storage_unit.hpp"
#include "ledger/testing/block_generator.hpp"
#include "muddle/create_muddle_fake.hpp"
#include "muddle/muddle_interface.hpp"
#include "shards/manifest_cache_interface.hpp"

#include "gtest/gtest.h"

#include <numeric>

using namespace fetch;
using namespace fetch::beacon;
using namespace fetch::dkg;
using namespace fetch::ledger;
using namespace fetch::ledger::testing;

using Address = fetch::muddle::Packet::Address;
struct DummyManifestCache : public fetch::shards::ManifestCacheInterface
{
  bool QueryManifest(Address const & /*address*/, fetch::shards::Manifest & /*manifest*/) override
  {
    return false;
  }
};

using Certificate    = fetch::crypto::Prover;
using CertificatePtr = std::shared_ptr<Certificate>;
using BlockPtr       = Consensus::NextBlockPtr;
using Muddle         = muddle::MuddlePtr;
using ConstByteArray = byte_array::ConstByteArray;
using MuddleAddress  = ConstByteArray;

struct NotarisationNode
{
  uint16_t                                   muddle_port;
  EventManager::SharedEventManager           event_manager;
  network::NetworkManager                    network_manager;
  core::Reactor                              reactor;
  ProverPtr                                  muddle_certificate;
  Muddle                                     muddle;
  DummyManifestCache                         manifest_cache;
  MainChain                                  chain;
  FakeStorageUnit                            storage_unit;
  std::shared_ptr<TrustedDealerSetupService> beacon_setup_service;
  std::shared_ptr<BeaconService>             beacon_service;
  std::shared_ptr<NotarisationService>       notarisation_service;
  std::shared_ptr<StakeManager>              stake_manager;
  Consensus                                  consensus;

  NotarisationNode(uint16_t port_number, uint16_t index, uint64_t cabinet_size,
                   uint64_t aeon_period, double threshold)
    : muddle_port{port_number}
    , event_manager{EventManager::New()}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{muddle::CreateMuddleFake("Test", muddle_certificate, network_manager, "127.0.0.1")}
    , chain{false, ledger::MainChain::Mode::IN_MEMORY_DB}
    , beacon_setup_service{new TrustedDealerSetupService{
          *muddle, manifest_cache, muddle_certificate, threshold, aeon_period}}
    , beacon_service{new BeaconService{*muddle, muddle_certificate, *beacon_setup_service,
                                       event_manager}}
    , notarisation_service{new NotarisationService{*muddle, muddle_certificate,
                                                   *beacon_setup_service}}
    , stake_manager{new StakeManager{}}
    , consensus{stake_manager,  beacon_setup_service,
                beacon_service, chain,
                storage_unit,   muddle_certificate->identity(),
                aeon_period,    cabinet_size,
                1000,           notarisation_service}
  {
    network_manager.Start();
    muddle->Start({muddle_port});
  }

  ~NotarisationNode()
  {
    reactor.Stop();
    muddle->Stop();
    network_manager.Stop();
  }

  MuddleAddress address()
  {
    return muddle_certificate->identity().identifier();
  }

  network::Uri GetHint() const
  {
    return fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(muddle_port)};
  }
};

TEST(notarisation, notarise_blocks)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  uint32_t num_nodes    = 6;
  uint32_t cabinet_size = 3;
  double   threshold    = 0.5;
  uint64_t aeon_period  = 5;
  uint64_t stake        = 10;

  std::vector<std::shared_ptr<NotarisationNode>> nodes;
  for (uint16_t i = 0; i < num_nodes; ++i)
  {
    auto port_number = static_cast<uint16_t>(10000 + i);
    nodes.emplace_back(new NotarisationNode(port_number, i, cabinet_size, aeon_period, threshold));
  }

  // Connect muddles together
  for (uint32_t i = 0; i < num_nodes; i++)
  {
    for (uint32_t j = i + 1; j < num_nodes; j++)
    {
      nodes[i]->muddle->ConnectTo(nodes[j]->address(), nodes[j]->GetHint());
    }
  }

  // Wait for all the nodes to completely connect
  std::vector<uint32_t> pending_nodes(num_nodes, 0);
  std::iota(pending_nodes.begin(), pending_nodes.end(), 0);
  while (!pending_nodes.empty())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (auto it = pending_nodes.begin(); it != pending_nodes.end();)
    {
      auto &muddle = *(nodes[*it]->muddle);

      if ((num_nodes - 1) <= muddle.GetNumDirectlyConnectedPeers())
      {
        it = pending_nodes.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  // Attach runnables
  for (auto &node : nodes)
  {
    node->reactor.Attach(node->beacon_setup_service->GetWeakRunnables());
    node->reactor.Attach(node->beacon_service->GetWeakRunnable());
    node->reactor.Attach(node->notarisation_service->GetWeakRunnable());
  }

  // Start reactor
  for (auto &node : nodes)
  {
    node->reactor.Start();
    node->consensus.SetMaxCabinetSize(static_cast<uint16_t>(cabinet_size));
  }

  // Stake setup
  StakeSnapshot           snapshot;
  std::set<MuddleAddress> cabinet;
  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    snapshot.UpdateStake(nodes[i]->muddle_certificate->identity(), stake);
    cabinet.insert(nodes[i]->muddle_certificate->identity().identifier());
  }
  EXPECT_EQ(snapshot.total_stake(), cabinet_size * stake);

  // Completely change over committee and queue updates
  for (auto &node : nodes)
  {
    node->consensus.Reset(snapshot, node->storage_unit);
    for (uint32_t j = 0; j < num_nodes; ++j)
    {
      if (j >= cabinet_size)
      {
        node->consensus.stake()->update_queue().AddStakeUpdate(
            4, nodes[j]->muddle_certificate->identity(), stake);
      }
      else
      {
        node->consensus.stake()->update_queue().AddStakeUpdate(
            4, nodes[j]->muddle_certificate->identity(), 0);
      }
    }
  }

  // Setup trusted dealer for first aeon
  TrustedDealer dealer{cabinet, threshold};

  // Reset cabinet with ready made keys
  uint64_t round_start = 1;
  uint64_t start_time =
      GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM)) + 5;
  BlockEntropy prev_entropy;
  prev_entropy.group_signature = "Hello";
  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    nodes[i]->beacon_setup_service->StartNewCabinet(
        cabinet, round_start, start_time, prev_entropy, dealer.GetDkgKeys(nodes[i]->address()),
        dealer.GetNotarisationKeys(nodes[i]->address()));
  }

  // Generate blocks and notarise for 2 aeons
  for (uint16_t block_number = 1; block_number < aeon_period * 2 + 1; block_number++)
  {
    std::vector<BlockPtr> blocks_this_round;
    uint32_t              count = 0;
    while (count == 0)
    {
      for (auto &node : nodes)
      {
        auto next_block = node->consensus.GenerateNextBlock();

        if (next_block != nullptr)
        {
          // Set block hash and ficticious weight for first block
          if (block_number == 1)
          {
            next_block->previous_hash = node->chain.GetHeaviestBlock()->hash;
            next_block->weight        = static_cast<uint64_t>(
                cabinet_size -
                std::distance(nodes.begin(), std::find(nodes.begin(), nodes.end(), node)));
          }

          next_block->UpdateDigest();
          next_block->UpdateTimestamp();
          next_block->miner_signature = node->muddle_certificate->Sign(next_block->hash);
          assert(next_block->weight != 0);

          blocks_this_round.push_back(std::move(next_block));
          ++count;
        }
      }
    }

    // Verify notarisation in block
    for (auto &node : nodes)
    {
      for (auto &block : blocks_this_round)
      {
        EXPECT_TRUE(node->consensus.VerifyNotarisation(*block));
      }
    }

    // Add this block to everyone's chain
    for (auto &node : nodes)
    {
      for (auto &block : blocks_this_round)
      {
        node->chain.AddBlock(*block);
        node->consensus.UpdateCurrentBlock(*block);
      }
    }
  }
}
