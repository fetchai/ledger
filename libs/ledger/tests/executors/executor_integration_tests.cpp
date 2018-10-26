//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
#include "crypto/prover.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/protocols/executor_rpc_client.hpp"
#include "ledger/protocols/executor_rpc_service.hpp"
#include "ledger/storage_unit/storage_unit_bundled_service.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "storage/resource_mapper.hpp"
#include "mock_storage_unit.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <thread>

using LaneIndex = fetch::ledger::StorageUnitClient::LaneIndex;

using ::testing::_;

class ExecutorIntegrationTests : public ::testing::Test
{
protected:
  using underlying_client_type          = fetch::ledger::ExecutorRpcClient;
  using underlying_service_type         = fetch::ledger::ExecutorRpcService;
  using underlying_network_manager_type = underlying_client_type::NetworkManager;
  using underlying_storage_type         = fetch::ledger::StorageUnitClient;
  using underlying_storage_service_type = fetch::ledger::StorageUnitBundledService;
  using TCPClient                       = fetch::network::TCPClient;
  using Peer                            = fetch::network::Peer;
  using ResourceID                      = fetch::storage::ResourceID;
  using ResourceAddress =                 fetch::storage::ResourceAddress;

  using client_type          = std::unique_ptr<underlying_client_type>;
  using service_type         = std::unique_ptr<underlying_service_type>;
  using network_manager_type = std::unique_ptr<underlying_network_manager_type>;
  using storage_client_type  = std::shared_ptr<underlying_storage_type>;
  using storage_service_type = std::unique_ptr<underlying_storage_service_type>;
  using rng_type             = std::mt19937;

  using Muddle    = fetch::muddle::Muddle;
  using MuddlePtr = std::unique_ptr<Muddle>;
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

    using Signer    = fetch::crypto::ECDSASigner;
    using SignerPtr = std::unique_ptr<Signer>;

    SignerPtr certificate        = std::make_unique<Signer>();
    bool      certificate_loaded = false;

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

    network_manager_ = std::make_unique<underlying_network_manager_type>(2);
    network_manager_->Start();

    storage_service_ = std::make_unique<underlying_storage_service_type>();
    storage_service_->Setup("teststore", NUM_LANES, LANE_RPC_PORT_START, *network_manager_, true);
    storage_service_->Start();

    ProverPtr p2p_key = GenerateP2PKey();
    muddle_           = std::make_unique<Muddle>(std::move(p2p_key), *network_manager_);

    muddle_->Start({P2P_RPC_PORT});

    LaneIndex num_lanes = NUM_LANES;

    uint16_t lane_port_start = LANE_RPC_PORT_START;

    storage_.reset(new underlying_storage_type{*network_manager_, *muddle_});

    using InFlightCounter =
        fetch::network::AtomicInFlightCounter<fetch::network::AtomicCounterName::TCP_PORT_STARTUP>;

    fetch::network::FutureTimepoint deadline(std::chrono::seconds(40));
    if (!InFlightCounter::Wait(deadline))
    {
      throw std::runtime_error("Not all socket servers connected correctly. Aborting test");
    }

    std::map<LaneIndex, Uri> lane_data;
    for (LaneIndex i = 0; i < num_lanes; ++i)
    {
      uint16_t const lane_port = static_cast<uint16_t>(lane_port_start + i);
      lane_data[i] = fetch::network::Uri("tcp://127.0.0.1:" + std::to_string(lane_port));
    }

    // create the executor service
    service_ =
        std::make_unique<underlying_service_type>(EXECUTOR_RPC_PORT, *network_manager_, storage_);

    service_->Start();

    // create the executor client
    executor_ =
        std::make_unique<underlying_client_type>("127.0.0.1", EXECUTOR_RPC_PORT, *network_manager_);

    auto count = storage_->AddLaneConnectionsWaitingMuddle(*muddle_, lane_data,
                                                           std::chrono::milliseconds(30000));
    FETCH_LOG_WARN(LOGGING_NAME, "Lane connections established ", count, " of ", num_lanes);
    for (;;)
    {
      // wait for the all the clients to connect to everything
      if (executor_->is_alive())
      {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
  }

  void TearDown() override
  {
    service_->Stop();
    storage_service_->Stop();
    network_manager_->Stop();

    executor_.reset();
    service_.reset();
    storage_.reset();
    storage_service_.reset();
    network_manager_.reset();

    sleep(1);  // just give TCP time to settle.
  }

  fetch::chain::Transaction CreateDummyTransaction()
  {
    fetch::chain::MutableTransaction tx;
    tx.set_contract_name("fetch.dummy.wait");
    return fetch::chain::Transaction::Create(std::move(tx));
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

  fetch::chain::Transaction CreateWalletTransaction()
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
    fetch::chain::MutableTransaction tx;
    tx.set_contract_name("fetch.token.wealth");
    tx.set_data(oss.str());
    tx.PushResource(address);

    return fetch::chain::Transaction::Create(std::move(tx));
  }

  network_manager_type network_manager_;
  storage_service_type storage_service_;
  storage_client_type  storage_;
  service_type         service_;
  client_type          executor_;
  rng_type             rng_;

  MuddlePtr muddle_;
};

TEST_F(ExecutorIntegrationTests, CheckDummyContract)
{
  // create the dummy contract
  auto tx = CreateDummyTransaction();

  // store the transaction inside the store
  storage_->AddTransaction(tx);

  auto const status = executor_->Execute(tx.digest(), 0, {0});
  EXPECT_EQ(status, fetch::ledger::ExecutorInterface::Status::SUCCESS);
}

TEST_F(ExecutorIntegrationTests, CheckTokenContract)
{
  // create the dummy contract
  auto tx = CreateWalletTransaction();

  // store the transaction inside the store
  storage_->AddTransaction(tx);

  auto const status = executor_->Execute(tx.digest(), 0, {0});
  EXPECT_EQ(status, fetch::ledger::ExecutorInterface::Status::SUCCESS);
}
