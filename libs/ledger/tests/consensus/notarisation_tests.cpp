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

#include "ledger/shards/manifest_cache_interface.hpp"
#include "muddle/create_muddle_fake.hpp"
#include "muddle/muddle_interface.hpp"

#include "beacon/aeon.hpp"
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

struct DummyManifesttCache : public ManifestCacheInterface
{
  bool QueryManifest(Address const & /*address*/, Manifest & /*manifest*/) override
  {
    return false;
  }
};

using SharedAeonExecutionUnit = std::shared_ptr<AeonExecutionUnit>;

struct NotarisationNode
{
  using Muddle        = muddle::MuddlePtr;
  using MuddleAddress = byte_array::ConstByteArray;
  using BlockHash     = Digest;

  uint16_t                      muddle_port;
  network::NetworkManager       network_manager;
  core::Reactor                 reactor;
  ProverPtr                     muddle_certificate;
  Muddle                        muddle;
  DummyManifesttCache           manifest_cache;
  MainChain                     chain;
  NotarisationService           notarisation_service;
  std::unordered_set<BlockHash> notarised_blocks;

  NotarisationNode(uint16_t port_number, uint16_t index)
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{muddle::CreateMuddleFake("Test", muddle_certificate, network_manager, "127.0.0.1")}
    , chain{false, ledger::MainChain::Mode::IN_MEMORY_DB}
    , notarisation_service{*muddle, chain, muddle_certificate}
  {
    network_manager.Start();
    muddle->Start({muddle_port});

    notarisation_service.SetNotarisedBlockCallback(
        [this](BlockHash hash) { notarised_blocks.insert(hash); });
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

  void CreateNewAeonExeUnit(DkgOutput const &output)
  {
    SharedAeonExecutionUnit aeon_keys = std::make_shared<AeonExecutionUnit>();
    aeon_keys->manager.SetCertificate(muddle_certificate);
    aeon_keys->manager.SetDkgOutput(output);
    aeon_keys->aeon.round_start = 0;
    aeon_keys->aeon.round_end   = 10;
    notarisation_service.NewAeonExeUnit(aeon_keys);
  }
};

TEST(notarisation, notarise_blocks)
{
  // Set up identity and keys
  uint32_t committee_size = 3;
  uint32_t threshold      = 1;

  std::vector<std::shared_ptr<NotarisationNode>> nodes;
  std::set<NotarisationNode::MuddleAddress>      cabinet;
  for (uint16_t i = 0; i < committee_size; ++i)
  {
    auto port_number = static_cast<uint16_t>(10000 + i);
    nodes.emplace_back(new NotarisationNode(port_number, i));
    cabinet.insert(nodes[i]->address());
  }

  TrustedDealer dealer(cabinet, threshold);

  // Connect muddles together (localhost for this example)
  for (uint32_t ii = 0; ii < committee_size; ii++)
  {
    for (uint32_t jj = ii + 1; jj < committee_size; jj++)
    {
      nodes[ii]->muddle->ConnectTo(nodes[jj]->address(), nodes[jj]->GetHint());
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

  // For each node create an aeon execution unit and give keys to notarisation service
  std::vector<SharedAeonExecutionUnit> aeon_exe_units;
  for (uint32_t i = 0; i < committee_size; i++)
  {
    nodes[i]->CreateNewAeonExeUnit(dealer.GetKeys(nodes[i]->address()));
  }

  // Generate blocks!
  std::unordered_set<NotarisationNode::BlockHash> expected_notarisations;
  std::vector<BlockPtr>                           blocks_to_sign;
  BlockGenerator                                  generator(1, 1);
  for (uint16_t i = 0; i < 9; i++)
  {
    auto          random_miner = static_cast<uint32_t>(rand()) % committee_size;
    BlockPtrConst previous     = nodes[random_miner]->chain.GetHeaviestBlock();
    blocks_to_sign.push_back(generator(previous));
    blocks_to_sign[i]->body.block_entropy.qualified = cabinet;
    blocks_to_sign[i]->body.miner_id = nodes[random_miner]->muddle_certificate->identity();
    expected_notarisations.insert(blocks_to_sign[i]->body.hash);

    // Add this block to everyones chain
    for (auto &node : nodes)
    {
      node->chain.AddBlock(*blocks_to_sign[i]);
    }
  }

  // Start reactor
  for (auto &node : nodes)
  {
    node->reactor.Attach(node->notarisation_service.GetWeakRunnables());
    node->reactor.Start();
  }

  // Start signing
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  for (auto const &block : blocks_to_sign)
  {
    for (auto &node : nodes)
    {
      node->notarisation_service.NotariseBlock(block->body);
      EXPECT_TRUE(!node->notarisation_service.GetNotarisations(block->body.block_number).empty());
    }
  }

  // wait for all the nodes to completely connect
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
