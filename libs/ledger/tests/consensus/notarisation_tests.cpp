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

#include "ledger/chain/main_chain.hpp"
#include "ledger/protocols/notarisation_service.hpp"
#include "ledger/testing/block_generator.hpp"

#include "muddle/create_muddle_fake.hpp"
#include "muddle/muddle_interface.hpp"
#include "shards/manifest_cache_interface.hpp"

#include "beacon/beacon_setup_service.hpp"
#include "beacon/create_new_certificate.hpp"
#include "beacon/trusted_dealer.hpp"
#include "crypto/mcl_dkg.hpp"

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
using BlockPtr       = BlockGenerator::BlockPtr;
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
  uint16_t                             muddle_port;
  network::NetworkManager              network_manager;
  core::Reactor                        reactor;
  ProverPtr                            muddle_certificate;
  Muddle                               muddle;
  DummyManifestCache                   manifest_cache;
  MainChain                            chain;
  std::shared_ptr<NotarisationService> notarisation_service;
  BeaconSetupService                   beacon_setup_service;
  std::unordered_set<BlockHash>        notarised_blocks;
  std::atomic<bool>                    finished;
  DkgOutput                            output;

  NotarisationNode(uint16_t port_number, uint16_t index)
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{muddle::CreateMuddleFake("Test", muddle_certificate, network_manager, "127.0.0.1")}
    , chain{false, ledger::MainChain::Mode::IN_MEMORY_DB}
    , notarisation_service{new NotarisationService{*muddle, chain, muddle_certificate}}
    , beacon_setup_service{*muddle, muddle_certificate->identity(), manifest_cache,
                           muddle_certificate, notarisation_service}
  {
    network_manager.Start();
    muddle->Start({muddle_port});

    beacon_setup_service.SetBeaconReadyCallback([this](SharedAeonExecutionUnit beacon) -> void {
      finished = true;
      output   = beacon->manager.GetDkgOutput();
    });

    notarisation_service->SetNotarisedBlockCallback(
        [this](BlockHash hash) { notarised_blocks.insert(hash); });
  }

  ~NotarisationNode()
  {
    reactor.Stop();
    muddle->Stop();
    network_manager.Stop();
  }

  void QueueCabinet(std::set<MuddleAddress> cabinet, uint32_t threshold)
  {
    SharedAeonExecutionUnit beacon = std::make_shared<AeonExecutionUnit>();

    beacon->manager.SetCertificate(muddle_certificate);
    beacon->manager.NewCabinet(cabinet, threshold);

    // Setting the aeon details
    beacon->aeon.round_start = 0;
    beacon->aeon.round_end   = 10;
    beacon->aeon.members     = std::move(cabinet);
    // Plus 5 so tests pass on first DKG attempt
    beacon->aeon.start_reference_timepoint =
        GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM)) + 5;

    beacon_setup_service.QueueSetup(beacon);
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
  uint32_t threshold      = 1;

  std::vector<std::shared_ptr<NotarisationNode>> nodes;
  std::set<MuddleAddress>                        cabinet;
  for (uint16_t i = 0; i < committee_size; ++i)
  {
    auto port_number = static_cast<uint16_t>(10000 + i);
    nodes.emplace_back(new NotarisationNode(port_number, i));
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

  // Reset cabinet for rbc in pre-dkg sync
  for (auto &node : nodes)
  {
    node->QueueCabinet(cabinet, threshold);
  }

  // Attach runnables
  for (auto &node : nodes)
  {
    node->reactor.Attach(node->beacon_setup_service.GetWeakRunnables());
    node->reactor.Attach(node->notarisation_service->GetWeakRunnables());
  }

  // Start reactor
  for (auto &node : nodes)
  {
    node->reactor.Start();
  }

  // Loop until everyone we expect to finish completes the DKG
  pending_nodes.resize(committee_size, 0);
  std::iota(pending_nodes.begin(), pending_nodes.end(), 0);
  while (!pending_nodes.empty())
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    for (auto it = pending_nodes.begin(); it != pending_nodes.end();)
    {
      if (nodes[*it]->finished)
      {
        it = pending_nodes.erase(it);
        break;
      }
      ++it;
    }
  }

  // Generate blocks!
  std::unordered_set<BlockHash> expected_notarisations;
  std::vector<BlockPtr>         blocks_to_sign;
  BlockGenerator                generator(1, 1);
  for (uint16_t i = 0; i < 9; i++)
  {
    auto          random_miner = static_cast<uint32_t>(rand()) % committee_size;
    BlockPtrConst previous     = nodes[random_miner]->chain.GetHeaviestBlock();
    blocks_to_sign.push_back(generator(previous));
    blocks_to_sign[i]->body.block_entropy.qualified = cabinet;
    blocks_to_sign[i]->body.miner_id = nodes[random_miner]->muddle_certificate->identity();
    expected_notarisations.insert(blocks_to_sign[i]->body.hash);

    // Add this block to everyone's chain
    for (auto &node : nodes)
    {
      node->chain.AddBlock(*blocks_to_sign[i]);
    }
  }

  // Start signing
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  for (auto const &block : blocks_to_sign)
  {
    for (auto &node : nodes)
    {
      node->notarisation_service->NotariseBlock(block->body);
      EXPECT_TRUE(!node->notarisation_service->GetNotarisations(block->body.block_number).empty());
    }
  }

  // Wait for to complete notarisations
  pending_nodes.resize(committee_size, 0);
  std::iota(pending_nodes.begin(), pending_nodes.end(), 0);
  while (!pending_nodes.empty())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (auto it = pending_nodes.begin(); it != pending_nodes.end();)
    {
      if (nodes[*it]->notarised_blocks == expected_notarisations)
      {
        it = pending_nodes.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }
}
