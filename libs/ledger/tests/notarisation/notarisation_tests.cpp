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

using Prover         = fetch::crypto::Prover;
using ProverPtr      = std::shared_ptr<Prover>;
using Certificate    = fetch::crypto::Prover;
using CertificatePtr = std::shared_ptr<Certificate>;
using Address        = fetch::muddle::Packet::Address;
using BlockPtr       = Consensus::NextBlockPtr;
using BlockPtrConst  = BlockGenerator::BlockPtrConst;

struct DummyManifestCache : public fetch::shards::ManifestCacheInterface
{
  bool QueryManifest(Address const & /*address*/, fetch::shards::Manifest & /*manifest*/) override
  {
    return false;
  }
};

using Muddle                     = muddle::MuddlePtr;
using ConstByteArray             = byte_array::ConstByteArray;
using MuddleAddress              = ConstByteArray;
using BlockHash                  = Digest;
using AeonNotarisationUnit       = ledger::NotarisationManager;
using SharedAeonNotarisationUnit = std::shared_ptr<AeonNotarisationUnit>;
using SharedAeonExecutionUnit    = BeaconSetupService::SharedAeonExecutionUnit;

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
  std::shared_ptr<TrustedDealerSetupService> beacon_setup_service;
  std::shared_ptr<BeaconService>             beacon_service;
  std::shared_ptr<NotarisationService>       notarisation_service;
  std::shared_ptr<StakeManager>              stake_manager;
  Consensus                                  consensus;

  NotarisationNode(uint16_t port_number, uint16_t index, uint64_t cabinet_size)
    : muddle_port{port_number}
    , event_manager{EventManager::New()}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{muddle::CreateMuddleFake("Test", muddle_certificate, network_manager, "127.0.0.1")}
    , chain{false, ledger::MainChain::Mode::IN_MEMORY_DB}
    , beacon_setup_service{new TrustedDealerSetupService{*muddle, manifest_cache,
                                                         muddle_certificate}}
    , beacon_service{new BeaconService{*muddle, muddle_certificate, *beacon_setup_service,
                                       event_manager}}
    , notarisation_service{new NotarisationService{*muddle, chain, muddle_certificate,
                                                   *beacon_setup_service}}
    , stake_manager{new StakeManager{cabinet_size}}
    , consensus{stake_manager,
                beacon_setup_service,
                beacon_service,
                chain,
                muddle_certificate->identity(),
                10,
                cabinet_size,
                1000,
                notarisation_service}
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
  // Set up identity and keys
  uint32_t committee_size = 3;
  uint32_t threshold      = 2;

  std::vector<std::shared_ptr<NotarisationNode>> nodes;
  std::set<MuddleAddress>                        cabinet;
  for (uint16_t i = 0; i < committee_size; ++i)
  {
    auto port_number = static_cast<uint16_t>(10000 + i);
    nodes.emplace_back(new NotarisationNode(port_number, i, committee_size));
    cabinet.insert(nodes[i]->address());
  }

  // Connect muddles together (localhost for this example)
  for (uint32_t i = 0; i < committee_size; i++)
  {
    for (uint32_t j = i + 1; j < committee_size; j++)
    {
      nodes[i]->muddle->ConnectTo(nodes[j]->address(), nodes[j]->GetHint());
    }
  }

  // wait for all the nodes to completely connect
  std::vector<uint32_t> pending_nodes(committee_size, 0);
  std::iota(pending_nodes.begin(), pending_nodes.end(), 0);
  while (!pending_nodes.empty())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (auto it = pending_nodes.begin(); it != pending_nodes.end();)
    {
      auto &muddle = *(nodes[*it]->muddle);

      if ((committee_size - 1) <= muddle.GetNumDirectlyConnectedPeers())
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
  }

  // Stake setup
  StakeSnapshot snapshot;
  for (auto node : nodes)
  {
    snapshot.UpdateStake(node->muddle_certificate->identity(), 10);
    node->consensus.SetCabinetSize(committee_size);
    node->consensus.SetThreshold(0.5);
  }
  EXPECT_EQ(snapshot.total_stake(), committee_size * 10);

  uint64_t round_length = 5;
  for (uint8_t round = 0; round < 2; ++round)
  {
    // Setup trusted dealer
    TrustedDealer dealer{cabinet, threshold};

    // Create previous entropy
    BlockEntropy prev_entropy;
    if (round == 0)
    {
      prev_entropy.group_signature = "Hello";
    }
    else
    {
      prev_entropy = nodes[0]->chain.GetHeaviestBlock()->body.block_entropy;
    }

    // Reset cabinet for dkg
    uint64_t round_start = round * round_length + 1;
    for (auto &node : nodes)
    {
      node->consensus.stake()->Reset(snapshot);
      node->beacon_setup_service->StartNewCabinet(
          cabinet, threshold, round_start, round_start + round_length,
          GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM)) + 5,
          prev_entropy, dealer.GetDkgKeys(node->address()),
          dealer.GetNotarisationKeys(node->address()));
    }

    for (uint16_t i = 0; i < round_length; i++)
    {
      std::vector<BlockPtr> blocks_this_round;
      // Generate blocks and notarise
      uint32_t count = 0;
      while (count == 0)
      {
        for (auto &node : nodes)
        {
          auto next_block = node->consensus.GenerateNextBlock();

          if (next_block != nullptr)
          {

            // Set block hash and ficticious weight
            if (round == 0 && i == 0)
            {
              next_block->body.previous_hash = node->chain.GetHeaviestBlock()->body.hash;
              next_block->weight             = static_cast<uint64_t>(
                  committee_size -
                  std::distance(nodes.begin(), std::find(nodes.begin(), nodes.end(), node)));
            }

            next_block->UpdateDigest();
            next_block->UpdateTimestamp();
            next_block->miner_signature = node->muddle_certificate->Sign(next_block->body.hash);

            blocks_this_round.push_back(std::move(next_block));
            ++count;
          }
        }
      }

      std::cout << "Generated " << count << " blocks for round " << i << std::endl;

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
}
