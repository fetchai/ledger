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

#include "crypto/key_generator.hpp"
#include "ledger/genesis_loading/genesis_file_creator.hpp"
#include "shards/manifest.hpp"
#include "shards/service_identifier.hpp"
#include "network/peer.hpp"
#include "constellation/constellation.hpp"

#include "core/random/lcg.hpp"

#include "gmock/gmock.h"

using namespace fetch;
using namespace fetch::crypto;
using namespace fetch::ledger;
using namespace fetch::network;
using namespace fetch::constellation;
using namespace fetch::shards;

static constexpr uint16_t HTTP_PORT_OFFSET    = 0;
static constexpr uint16_t P2P_PORT_OFFSET     = 1;
static constexpr uint16_t DKG_PORT_OFFSET     = 2;
static constexpr uint16_t STORAGE_PORT_OFFSET = 10;

static constexpr uint16_t NUM_LANES = 2;

using fetch::network::Uri;
using UriSet       = fetch::constellation::Constellation::UriSet;

using Uris     = std::vector<Uri>;
using Manifest = shards::Manifest;

UriSet ToUriSet(Uris const &uris)
{
  UriSet s{};
  for (auto const &uri : uris)
  {
    s.emplace(uri);
  }

  return s;
}

// Helper to build the manifest
void BuildManifest(Manifest &manifest, uint16_t port, uint32_t num_lanes)
{
  // For this test, this is correct
  std::string const external_address = "127.0.0.1";

  Peer peer;

  // register the HTTP service
  peer.Update(external_address, port + HTTP_PORT_OFFSET);
  manifest.AddService(ServiceIdentifier{ServiceIdentifier::Type::HTTP}, ManifestEntry{peer});

  // register the P2P service
  peer.Update(external_address, static_cast<uint16_t>(port + P2P_PORT_OFFSET));
  manifest.AddService(ServiceIdentifier{ServiceIdentifier::Type::CORE}, ManifestEntry{peer});

  // register the DKG service
  peer.Update(external_address, static_cast<uint16_t>(port + DKG_PORT_OFFSET));
  manifest.AddService(ServiceIdentifier{ServiceIdentifier::Type::DKG}, ManifestEntry{peer});

  // register all of the lanes (storage shards)
  for (uint32_t i = 0; i < num_lanes; ++i)
  {
    peer.Update(external_address, static_cast<uint16_t>(port + STORAGE_PORT_OFFSET + (2 * i)));

    manifest.AddService(ServiceIdentifier{ServiceIdentifier::Type::LANE, static_cast<uint16_t>(i)},
                        ManifestEntry{peer});
  }
}

// Helper function to create the config for constellation
Constellation::Config BuildConstellationConfig(std::string const &genesis_file_loc, uint16_t start_port)
{
  Constellation::Config cfg;

  cfg.log2_num_lanes        = NUM_LANES;
  cfg.num_slices            = 2;
  cfg.num_executors         = 2;
  cfg.db_prefix             = std::string("unit_test_multiple_constellation_") + std::to_string(start_port);
  cfg.processor_threads     = 2;
  cfg.verification_threads  = 2;
  cfg.max_peers             = 200;
  cfg.transient_peers       = 1;
  cfg.block_interval_ms     = 5000;
  cfg.stake_delay_period    = 10;
  cfg.peers_update_cycle_ms = 100;
  cfg.network_mode          = Constellation::NetworkMode::PRIVATE_NETWORK;

  BuildManifest(cfg.manifest, start_port, 1 << cfg.log2_num_lanes);

  // Relevant to the test
  cfg.proof_of_stake        = true;
  cfg.kademlia_routing      = true;
  cfg.aeon_period           = 10;
  cfg.max_cabinet_size      = 200;
  cfg.disable_signing       = true;
  cfg.sign_broadcasts       = false;
  cfg.load_genesis_file     = true;
  cfg.genesis_file_location = genesis_file_loc;

  return cfg;
}

// A class to inherit from constellation so as to access private
// members
class ConstellationGetter final : public Constellation
{
public:

  ConstellationGetter(CertificatePtr certificate, Config config) : Constellation(certificate, config) {};

  MainChain &GetChain()
  {
    return chain_;
  };
};

class FullConstellationTests : public ::testing::Test
{
  using Certificates          = std::vector<std::shared_ptr<crypto::Prover>>;
  using Constellations        = std::vector<std::unique_ptr<ConstellationGetter>>;
  using RunThreads            = std::vector<std::unique_ptr<std::thread>>;
protected:

  void SetUp() override
  {
  }

  void TearDown() override
  {
    //ClearAll();
  }

  void StartNodes(uint32_t nodes, uint32_t of_which_are_miners)
  {
    (void)(nodes);
    (void)(of_which_are_miners);

    // Create the identities which the nodes will have
    for (uint32_t i = 0; i < nodes; ++i)
    {
      certificates_.emplace_back(GenerateP2PKey(true));
    }

    // All nodes must have the same genesis file, so create it beforehand
    CreateGenesisFile();

    // Create the nodes, notifying them of the genesis file
    // Note they will be connected to every node with a lower port on start
    for (std::size_t i = 0; i < nodes; ++i)
    {
      constellations_.emplace_back(std::make_unique<ConstellationGetter>(certificates_[i], BuildConstellationConfig(genesis_file_location_, uint16_t(8000 + (i * 100)))));
    }

    // start the nodes with varying delays in separate threads to represent
    // real life conditions
    for (std::size_t i = 0; i < nodes; ++i)
    {
      auto &constellations = constellations_;
      auto argh = lcg_();
      uint64_t random_milliseconds = uint64_t(argh) % 10000;

      auto cb = [&constellations, i, random_milliseconds]()
      {
        Uris peers;

        for (std::size_t j = 0; j < i; ++j)
        {
          byte_array::ConstByteArray peer{"tcp://127.0.0.1:" + std::to_string(uint16_t(8001 + (j * 100)))};
          peers.emplace_back(Uri{peer});
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(random_milliseconds));
        constellations[i]->Run(ToUriSet(peers));
      };

      run_threads_.push_back(std::make_unique<std::thread>(cb));

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 60 * 20));
  }

  void CheckIdenticalBlock(uint64_t /*block_number*/)
  {
//    for (std::size_t i = 0; i < constellations_.size(); ++i)
//    {
//      if(i == 0)
//      {
//
//      }
//    }
  }

  void CreateGenesisFile()
  {
    GenesisFileCreator::CreateFile(certificates_, genesis_file_location_, 5);
  }

private:
  std::string const genesis_file_location_ = "genesis_file_unit_test.json";
  Certificates      certificates_;
  Constellations    constellations_;
  RunThreads        run_threads_;
  random::LinearCongruentialGenerator lcg_;
};

// Check that the dag can consistently add nodes locally and advance the epochs
TEST_F(FullConstellationTests, CheckBlockGeneration)
{
  // Start the nodes
  StartNodes(7, 7);

  // Check all of them generate and settle on a blockchain
  CheckIdenticalBlock(10);

  // Shut them down
  //ClearAll();

  std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  // This function has assertions
  //PopulateDAG();
}

