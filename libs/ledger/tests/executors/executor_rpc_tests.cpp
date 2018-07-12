#include "core/make_unique.hpp"
#include "core/byte_array/encoders.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/protocols/executor_rpc_client.hpp"
#include "ledger/protocols/executor_rpc_service.hpp"

#include "mock_storage_unit.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <chrono>
#include <thread>

using ::testing::_;

class ExecutorRpcTests : public ::testing::Test {
protected:
  using underlying_client_type = fetch::ledger::ExecutorRpcClient;
  using underlying_service_type = fetch::ledger::ExecutorRpcService;
  using underlying_network_manager_type = underlying_client_type::thread_manager_type;
  using client_type = std::unique_ptr<underlying_client_type>;
  using service_type = std::unique_ptr<underlying_service_type>;
  using network_manager_type = std::unique_ptr<underlying_network_manager_type>;
  using underlying_storage_type = MockStorageUnit;
  using storage_type = std::shared_ptr<underlying_storage_type>;
  using rng_type = std::mt19937;

  static constexpr std::size_t IDENTITY_SIZE = 64;

  ExecutorRpcTests() {
    rng_.seed(42);
  }

  void SetUp() override {
    static const uint16_t EXECUTOR_RPC_PORT = 9001;

    storage_.reset(new underlying_storage_type);
    network_manager_.reset(new underlying_network_manager_type{2});
    network_manager_->Start();

    // create the executor service
    service_.reset(new underlying_service_type{
      EXECUTOR_RPC_PORT,
      *network_manager_,
      storage_
    });

    // create the executor client
    executor_.reset(new underlying_client_type{
      "127.0.0.1",
      EXECUTOR_RPC_PORT,
      *network_manager_
    });

    // wait for the executor to connect etc.
    while (!executor_->is_alive()) {
      std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
  }

  void TearDown() override {
    network_manager_->Stop();

    executor_.reset();
    service_.reset();
    network_manager_.reset();
  }

  fetch::chain::Transaction CreateDummyTransaction() {
    fetch::chain::MutableTransaction tx;
    tx.set_contract_name("fetch.dummy.wait");
    return fetch::chain::VerifiedTransaction::Create(std::move(tx));
  }

  fetch::byte_array::ConstByteArray CreateAddress() {
    fetch::byte_array::ByteArray address;
    address.Resize(std::size_t{IDENTITY_SIZE});

    for (std::size_t i = 0; i < std::size_t{IDENTITY_SIZE}; ++i) {
      address[i] = static_cast<uint8_t>(rng_() & 0xFF);
    }

    return {address};
  }

  fetch::chain::Transaction CreateWalletTransaction() {

    // generate an address
    auto address = CreateAddress();

    // format the transaction contents
    std::ostringstream oss;
    oss << "{ "
        << R"("address": ")" << static_cast<std::string>(fetch::byte_array::ToBase64(address)) << "\", "
        << R"("amount": )" << 1000
        << " }";

    // create the transaction
    fetch::chain::MutableTransaction tx;
    tx.set_contract_name("fetch.token.wealth");
    tx.set_data(oss.str());

    return fetch::chain::VerifiedTransaction::Create(std::move(tx));
  }

  network_manager_type network_manager_;
  storage_type storage_;
  service_type service_;
  client_type executor_;
  rng_type rng_;
};

TEST_F(ExecutorRpcTests, CheckDummyContract) {

  EXPECT_CALL(*storage_, AddTransaction(_))
    .Times(1);
  EXPECT_CALL(*storage_, GetTransaction(_, _))
    .Times(1);

  // create the dummy contract
  auto tx = CreateDummyTransaction();

  // store the transaction inside the store
  storage_->AddTransaction(tx);

  auto const status = executor_->Execute(tx.digest(), 0, {0});
  EXPECT_EQ(status, fetch::ledger::ExecutorInterface::Status::SUCCESS);
}

TEST_F(ExecutorRpcTests, CheckTokenContract) {

  EXPECT_CALL(*storage_, AddTransaction(_))
    .Times(1);
  EXPECT_CALL(*storage_, GetTransaction(_, _))
    .Times(1);
  EXPECT_CALL(*storage_, GetOrCreate(_))
    .Times(1);
  EXPECT_CALL(*storage_, Set(_, _))
    .Times(1);

  // create the dummy contract
  auto tx = CreateWalletTransaction();

  // store the transaction inside the store
  storage_->AddTransaction(tx);

  auto const status = executor_->Execute(tx.digest(), 0, {0});
  EXPECT_EQ(status, fetch::ledger::ExecutorInterface::Status::SUCCESS);
}
