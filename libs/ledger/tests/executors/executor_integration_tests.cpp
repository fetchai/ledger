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
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/protocols/executor_rpc_client.hpp"
#include "ledger/protocols/executor_rpc_service.hpp"
#include "ledger/storage_unit/storage_unit_bundled_service.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"

#include "mock_storage_unit.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <thread>

using ::testing::_;

class ExecutorIntegrationTests : public ::testing::Test
{
protected:
  using underlying_client_type          = fetch::ledger::ExecutorRpcClient;
  using underlying_service_type         = fetch::ledger::ExecutorRpcService;
  using underlying_network_manager_type = underlying_client_type::network_manager_type;
  using underlying_storage_type         = fetch::ledger::StorageUnitClient;
  using underlying_storage_service_type = fetch::ledger::StorageUnitBundledService;

  using client_type          = std::unique_ptr<underlying_client_type>;
  using service_type         = std::unique_ptr<underlying_service_type>;
  using network_manager_type = std::unique_ptr<underlying_network_manager_type>;
  using storage_client_type  = std::shared_ptr<underlying_storage_type>;
  using storage_service_type = std::unique_ptr<underlying_storage_service_type>;
  using rng_type             = std::mt19937;

  static constexpr std::size_t IDENTITY_SIZE = 64;

  ExecutorIntegrationTests() { rng_.seed(42); }

  void SetUp() override
  {
    static const uint16_t    EXECUTOR_RPC_PORT   = 9000;
    static const uint16_t    LANE_RPC_PORT_START = 9001;
    static const std::size_t NUM_LANES           = 4;

    network_manager_ = std::make_unique<underlying_network_manager_type>(2);
    network_manager_->Start();

    storage_service_ = std::make_unique<underlying_storage_service_type>();
    storage_service_->Setup("teststore", NUM_LANES, LANE_RPC_PORT_START, *network_manager_, false);

    storage_.reset(new underlying_storage_type{*network_manager_});
    for (std::size_t i = 0; i < NUM_LANES; ++i)
    {
      storage_->AddLaneConnection<fetch::network::TCPClient>("localhost",
                                                             uint16_t(LANE_RPC_PORT_START + i));
    }

    // create the executor service
    service_ =
        std::make_unique<underlying_service_type>(EXECUTOR_RPC_PORT, *network_manager_, storage_);

    // create the executor client
    executor_ =
        std::make_unique<underlying_client_type>("127.0.0.1", EXECUTOR_RPC_PORT, *network_manager_);

    for (;;)
    {

      // wait for the all the clients to connect to everything
      if (executor_->is_alive() && storage_->is_alive()) break;

      std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
  }

  void TearDown() override
  {
    network_manager_->Stop();

    executor_.reset();
    service_.reset();
    storage_.reset();
    storage_service_.reset();
    network_manager_.reset();
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

  fetch::logger.Info("#### Adding transaction...");

  // store the transaction inside the store
  storage_->AddTransaction(tx);

  fetch::logger.Info("#### Executing transaction...");

  auto const status = executor_->Execute(tx.digest(), 0, {0});
  EXPECT_EQ(status, fetch::ledger::ExecutorInterface::Status::SUCCESS);
}
