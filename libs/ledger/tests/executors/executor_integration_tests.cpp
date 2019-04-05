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

#include "core/byte_array/encoders.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/protocols/executor_rpc_client.hpp"
#include "ledger/protocols/executor_rpc_service.hpp"
#include "ledger/storage_unit/storage_unit_bundled_service.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "mock_storage_unit.hpp"
#include "storage/resource_mapper.hpp"
#include "vectorise/meta/log2.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <thread>

using LaneIndex    = fetch::ledger::LaneIdentity::lane_type;
using ShardConfigs = fetch::ledger::ShardConfigs;

class ExecutorIntegrationTests : public ::testing::Test
{
protected:
  using ExecutorRpcClient         = fetch::ledger::ExecutorRpcClient;
  using ExecutorRpcService        = fetch::ledger::ExecutorRpcService;
  using NetworkManager            = fetch::network::NetworkManager;
  using StorageUnitClient         = fetch::ledger::StorageUnitClient;
  using StorageUnitBundledService = fetch::ledger::StorageUnitBundledService;
  using TCPClient                 = fetch::network::TCPClient;
  using Peer                      = fetch::network::Peer;
  using ResourceID                = fetch::storage::ResourceID;
  using ResourceAddress           = fetch::storage::ResourceAddress;

  using ExecutorRpcClientPtr         = std::shared_ptr<ExecutorRpcClient>;
  using ExecutorRpcServicePtr        = std::shared_ptr<ExecutorRpcService>;
  using NetworkManagerPtr            = std::shared_ptr<NetworkManager>;
  using StorageUnitClientPtr         = std::shared_ptr<StorageUnitClient>;
  using StorageUnitBundledServicePtr = std::shared_ptr<StorageUnitBundledService>;
  using rng_type                     = std::mt19937;

  using Muddle    = fetch::muddle::Muddle;
  using MuddlePtr = std::shared_ptr<Muddle>;
  using Prover    = fetch::crypto::Prover;
  using ProverPtr = std::unique_ptr<Prover>;
  using Uri       = fetch::network::Uri;

  static constexpr std::size_t IDENTITY_SIZE = 64;

  static constexpr char const *LOGGING_NAME = "ExecutorIntegrationTests";

  ExecutorIntegrationTests()
  {
    rng_.seed(42);
  }

  ProverPtr GenerateP2PKey()
  {
    static constexpr char const *KEY_FILENAME = "p2p.key";

    using Signer = fetch::crypto::ECDSASigner;

    auto certificate        = std::make_unique<Signer>();
    bool certificate_loaded = false;

    // Step 1. Attempt to load the existing key
    {
      std::ifstream input_file(KEY_FILENAME, std::ios::in | std::ios::binary);

      if (input_file.is_open())
      {
        fetch::byte_array::ByteArray private_key_data;
        private_key_data.Resize(Signer::PrivateKey::ecdsa_curve_type::privateKeySize);

        // attempt to read in the private key
        input_file.read(private_key_data.char_pointer(),
                        static_cast<std::streamsize>(private_key_data.size()));

        if (!(input_file.fail() || input_file.eof()))
        {
          certificate->Load(private_key_data);
          certificate_loaded = true;
        }
      }
    }

    // Generate a key if the load failed
    if (!certificate_loaded)
    {
      certificate->GenerateKeys();

      std::ofstream output_file(KEY_FILENAME, std::ios::out | std::ios::binary);

      if (output_file.is_open())
      {
        auto const private_key_data = certificate->private_key();

        output_file.write(private_key_data.char_pointer(),
                          static_cast<std::streamsize>(private_key_data.size()));
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to save P2P key");
      }
    }

    return certificate;
  }

  void SetUp() override
  {
    static const uint16_t    EXECUTOR_RPC_PORT   = 9120;
    static const uint16_t    P2P_RPC_PORT        = 9130;
    static const uint16_t    LANE_RPC_PORT_START = 9141;
    static const std::size_t NUM_LANES           = 4;
    static const std::size_t LOG2_NUM_LANES      = fetch::meta::Log2<NUM_LANES>::value;

    using fetch::muddle::NetworkId;

    // --- Start a NETWORK MANAGER ----------------------------------

    network_manager_ = std::make_shared<NetworkManager>("NetMgr", 10);
    network_manager_->Start();

    // --- Start the MUDDLE on top of the NETWORK MANAGER -----------

    ProverPtr p2p_key = GenerateP2PKey();
    muddle_ = Muddle::CreateMuddle(NetworkId{"Test"}, std::move(p2p_key), *network_manager_);
    muddle_->Start({P2P_RPC_PORT});

    // --- Start the STORAGE SERVICE --------------------------------

    ShardConfigs shards;
    for (std::size_t i = 0; i < NUM_LANES; ++i)
    {
      using fetch::ledger::ShardConfig;
      using fetch::muddle::NetworkId;
      using fetch::crypto::ECDSASigner;

      ShardConfig cfg;
      cfg.lane_id             = 0;
      cfg.num_lanes           = 1;
      cfg.storage_path        = "exec_int_tests";
      cfg.external_identity   = std::make_shared<ECDSASigner>();
      cfg.external_port       = static_cast<uint16_t>(LANE_RPC_PORT_START + (i * 2));
      cfg.external_network_id = NetworkId{"EXT-"};
      cfg.internal_identity   = std::make_shared<ECDSASigner>();
      cfg.internal_port       = static_cast<uint16_t>(LANE_RPC_PORT_START + (i * 2) + 1u);
      cfg.internal_network_id = NetworkId{"INT-"};

      shards.push_back(cfg);
    }

    storage_service_ = std::make_shared<StorageUnitBundledService>();
    storage_service_->Setup(*network_manager_, shards, false,
                            StorageUnitBundledService::Mode::CREATE_DATABASE);
    storage_service_->Start();

    storage_.reset(new StorageUnitClient{muddle_->AsEndpoint(), shards,
                                         static_cast<uint32_t>(LOG2_NUM_LANES)});

    // --- Start the EXECUTOR SERVICE -------------------------------

    auto executor_muddle = Muddle::CreateMuddle(NetworkId{"Test"}, *network_manager_);
    executor_service_ =
        std::make_shared<ExecutorRpcService>(EXECUTOR_RPC_PORT, storage_, executor_muddle);
    executor_service_->Start();

    // --- Wait for all the services to open listening ports --------

    using InFlightCounter =
        fetch::network::AtomicInFlightCounter<fetch::network::AtomicCounterName::TCP_PORT_STARTUP>;
    fetch::network::FutureTimepoint deadline(std::chrono::seconds(30));
    if (!InFlightCounter::Wait(deadline))
    {
      throw std::runtime_error("Not all socket servers connected correctly. Aborting test");
    }

    // --- Schedule lanes for connection ----------------------------

    LaneIndex                num_lanes       = NUM_LANES;
    uint16_t                 lane_port_start = LANE_RPC_PORT_START;
    std::map<LaneIndex, Uri> lane_data;
    for (LaneIndex i = 0; i < num_lanes; ++i)
    {
      uint16_t const lane_port = static_cast<uint16_t>(lane_port_start + i);
      lane_data[i] = fetch::network::Uri("tcp://127.0.0.1:" + std::to_string(lane_port));
    }

    // --- Schedule executor for connection ---------------------

    executor_ = std::make_shared<ExecutorRpcClient>(*network_manager_, *muddle_);
    executor_->Connect(*muddle_, Uri("tcp://127.0.0.1:" + std::to_string(EXECUTOR_RPC_PORT)));

    // --- Wait for connections to finish -----------------------

    using LocalServiceConnectionsCounter = fetch::network::AtomicInFlightCounter<
        fetch::network::AtomicCounterName::LOCAL_SERVICE_CONNECTIONS>;
    if (!LocalServiceConnectionsCounter::Wait(
            fetch::network::FutureTimepoint(std::chrono::seconds(30))))
    {
      throw std::runtime_error("Not all local services connected correctly. Aborting test");
    }

    size_t exec_count = executor_->connections();
    size_t lane_count = storage_->num_lanes();

    FETCH_LOG_WARN(LOGGING_NAME, "Lane connections established ", lane_count, " of ", num_lanes);
    FETCH_LOG_WARN(LOGGING_NAME, "Executor connections established ", exec_count, " of ", 1);
  }

  void TearDown() override
  {
    executor_service_->Stop();
    storage_service_->Stop();
    network_manager_->Stop();

    executor_.reset();
    executor_service_.reset();
    storage_.reset();
    storage_service_.reset();
    network_manager_.reset();

    sleep(1);  // just give TCP time to settle.
  }

  fetch::ledger::Transaction CreateDummyTransaction()
  {
    fetch::ledger::MutableTransaction tx;
    tx.set_contract_name("fetch.dummy.wait");
    return fetch::ledger::Transaction::Create(std::move(tx));
  }

  fetch::byte_array::ConstByteArray CreateAddress()
  {
    fetch::byte_array::ByteArray address;
    address.Resize(std::size_t{IDENTITY_SIZE});

    for (std::size_t i = 0; i < std::size_t{IDENTITY_SIZE}; ++i)
    {
      address[i] = static_cast<uint8_t>(rng_() & 0xFF);
    }

    return {address};
  }

  fetch::ledger::Transaction CreateWalletTransaction()
  {

    // generate an address
    auto address = CreateAddress();

    // format the transaction contents
    std::ostringstream oss;
    oss << "{ "
        << R"("address": ")" << static_cast<std::string>(fetch::byte_array::ToBase64(address))
        << "\", "
        << R"("amount": )" << 1000 << " }";

    // create the transaction
    fetch::ledger::MutableTransaction tx;
    tx.set_contract_name("fetch.token.wealth");
    tx.set_data(oss.str());
    tx.PushResource(address);

    return fetch::ledger::Transaction::Create(std::move(tx));
  }

  NetworkManagerPtr            network_manager_;
  StorageUnitBundledServicePtr storage_service_;
  StorageUnitClientPtr         storage_;
  ExecutorRpcServicePtr        executor_service_;
  ExecutorRpcClientPtr         executor_;
  rng_type                     rng_;

  MuddlePtr muddle_;
};

TEST_F(ExecutorIntegrationTests, DISABLED_CheckDummyContract)
{
  // create the dummy contract
  auto tx = CreateDummyTransaction();

  // store the transaction inside the store
  storage_->AddTransaction(tx);

  auto const status = executor_->Execute(tx.digest(), 0, {0});
  EXPECT_EQ(status, fetch::ledger::ExecutorInterface::Status::SUCCESS);
}

TEST_F(ExecutorIntegrationTests, DISABLED_CheckTokenContract)
{
  // create the dummy contract
  auto tx = CreateWalletTransaction();

  // store the transaction inside the store
  storage_->AddTransaction(tx);

  auto const status = executor_->Execute(tx.digest(), 0, {0});
  EXPECT_EQ(status, fetch::ledger::ExecutorInterface::Status::SUCCESS);
}
