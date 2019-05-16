////------------------------------------------------------------------------------
////
////   Copyright 2018-2019 Fetch.AI Limited
////
////   Licensed under the Apache License, Version 2.0 (the "License");
////   you may not use this file except in compliance with the License.
////   You may obtain a copy of the License at
////
////       http://www.apache.org/licenses/LICENSE-2.0
////
////   Unless required by applicable law or agreed to in writing, software
////   distributed under the License is distributed on an "AS IS" BASIS,
////   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
////   See the License for the specific language governing permissions and
////   limitations under the License.
////
////------------------------------------------------------------------------------
//
//#include "core/byte_array/encoders.hpp"
//#include "core/future_timepoint.hpp"
//#include "crypto/prover.hpp"
//#include "ledger/chain/mutable_transaction.hpp"
//#include "ledger/chain/transaction.hpp"
//#include "ledger/protocols/executor_rpc_client.hpp"
//#include "ledger/protocols/executor_rpc_service.hpp"
//#include "ledger/storage_unit/storage_unit_bundled_service.hpp"
//#include "ledger/storage_unit/storage_unit_client.hpp"
//#include "network/generics/atomic_inflight_counter.hpp"
//#include "storage/resource_mapper.hpp"
//#include "ledger/chain/v2/transaction_builder.hpp"
//
//#include "mock_storage_unit.hpp"
//
//#include <chrono>
//#include <gtest/gtest.h>
//#include <memory>
//#include <random>
//#include <thread>
//
// using ::testing::_;
// using fetch::network::AtomicInFlightCounter;
// using fetch::network::AtomicCounterName;
// using fetch::muddle::NetworkId;
// using fetch::ledger::v2::Transaction;
// using fetch::ledger::v2::TransactionBuilder;
// using fetch::ledger::v2::Address;
//
// class ExecutorRpcTests : public ::testing::Test
//{
// protected:
//  using ExecutorRpcClient  = fetch::ledger::ExecutorRpcClient;
//  using ExecutorRpcService = fetch::ledger::ExecutorRpcService;
//  using NetworkManager     = fetch::network::NetworkManager;
//
//  using ExecutorRpcClientPtr  = std::unique_ptr<ExecutorRpcClient>;
//  using ExecutorRpcServicePtr = std::unique_ptr<ExecutorRpcService>;
//  using NetworkManagerPtr     = std::unique_ptr<NetworkManager>;
//
//  using StorageUnit    = MockStorageUnit;
//  using StorageUnitPtr = std::shared_ptr<StorageUnit>;
//  using rng_type       = std::mt19937;
//
//  static constexpr std::size_t IDENTITY_SIZE = 64;
//
//  using Muddle    = fetch::muddle::Muddle;
//  using MuddlePtr = std::shared_ptr<Muddle>;
//  using Prover    = fetch::crypto::Prover;
//  using ProverPtr = std::shared_ptr<Prover>;
//  using Uri       = fetch::network::Uri;
//
//  static constexpr char const *LOGGING_NAME = "ExecutorRpcTests";
//
//  ExecutorRpcTests()
//  {
//    rng_.seed(42);
//  }
//
//  ProverPtr GenerateP2PKey()
//  {
//    static constexpr char const *KEY_FILENAME = "p2p.key";
//
//    using Signer    = fetch::crypto::ECDSASigner;
//    using SignerPtr = std::unique_ptr<Signer>;
//
//    SignerPtr certificate        = std::make_unique<Signer>();
//    bool      certificate_loaded = false;
//
//    // Step 1. Attempt to load the existing key
//    {
//      std::ifstream input_file(KEY_FILENAME, std::ios::in | std::ios::binary);
//
//      if (input_file.is_open())
//      {
//        fetch::byte_array::ByteArray private_key_data;
//        private_key_data.Resize(Signer::PrivateKey::ecdsa_curve_type::privateKeySize);
//
//        // attempt to read in the private key
//        input_file.read(private_key_data.char_pointer(),
//                        static_cast<std::streamsize>(private_key_data.size()));
//
//        if (!(input_file.fail() || input_file.eof()))
//        {
//          certificate->Load(private_key_data);
//          certificate_loaded = true;
//        }
//      }
//    }
//
//    // Generate a key if the load failed
//    if (!certificate_loaded)
//    {
//      certificate->GenerateKeys();
//
//      std::ofstream output_file(KEY_FILENAME, std::ios::out | std::ios::binary);
//
//      if (output_file.is_open())
//      {
//        auto const private_key_data = certificate->private_key();
//
//        output_file.write(private_key_data.char_pointer(),
//                          static_cast<std::streamsize>(private_key_data.size()));
//      }
//      else
//      {
//        FETCH_LOG_WARN(LOGGING_NAME, "Failed to save P2P key");
//      }
//    }
//
//    return certificate;
//  }
//
//  void SetUp() override
//  {
//    static const uint16_t EXECUTOR_RPC_PORT = 9111;
//    static const uint16_t P2P_RPC_PORT      = 9130;
//
//    storage_.reset(new StorageUnit);
//
//    // --- Start a NETWORK MANAGER ----------------------------------
//
//    network_manager_ = std::make_unique<NetworkManager>("NetMgr", 2);
//    network_manager_->Start();
//
//    // --- Start the MUDDLE on top of the NETWORK MANAGER -----------
//
//    prover_ = GenerateP2PKey();
//    muddle_ = Muddle::CreateMuddle(NetworkId{"Test"}, prover_, *network_manager_);
//    muddle_->Start({P2P_RPC_PORT});
//
//    // --- Start the EXECUTOR SERVICE -------------------------------
//
//    auto executor_muddle = Muddle::CreateMuddle(NetworkId{"Test"}, *network_manager_);
//    executor_service_ =
//        std::make_unique<ExecutorRpcService>(EXECUTOR_RPC_PORT, storage_, executor_muddle);
//    executor_service_->Start();
//
//    // --- Wait for all the services to open listening ports --------
//
//    using InFlightCounter =
//        fetch::network::AtomicInFlightCounter<fetch::network::AtomicCounterName::TCP_PORT_STARTUP>;
//    fetch::core::FutureTimepoint deadline(std::chrono::seconds(30));
//    if (!InFlightCounter::Wait(deadline))
//    {
//      throw std::runtime_error("Not all socket servers connected correctly. Aborting test");
//    }
//
//    // --- Schedule executor for connection ---------------------
//
//    executor_ = std::make_unique<ExecutorRpcClient>(*muddle_);
//    executor_->Connect(*muddle_, Uri("tcp://127.0.0.1:" + std::to_string(EXECUTOR_RPC_PORT)));
//
//    // --- Wait for connections to finish -----------------------
//
//    using LocalServiceConnectionsCounter = fetch::network::AtomicInFlightCounter<
//        fetch::network::AtomicCounterName::LOCAL_SERVICE_CONNECTIONS>;
//    if (!LocalServiceConnectionsCounter::Wait(
//            fetch::core::FutureTimepoint(std::chrono::seconds(30))))
//    {
//      throw std::runtime_error("Not all local services connected correctly. Aborting test");
//    }
//
//    size_t exec_count = executor_->connections();
//    FETCH_LOG_WARN(LOGGING_NAME, "Executor connections established ", exec_count, " of ", 1);
//  }
//
//  void TearDown() override
//  {
//    executor_service_->Stop();
//    network_manager_->Stop();
//
//    executor_.reset();
//    executor_service_.reset();
//    network_manager_.reset();
//  }
//
//  std::shared_ptr<Transaction> CreateDummyTransaction()
//  {
//    return TransactionBuilder()
//      .From(Address{prover_->identity()})
//      .TargetChainCode("fetch.dummy", fetch::BitVector{})
//      .Action("wait")
//      .Signer(prover_->identity())
//      .Seal()
//      .Sign(*prover_)
//      .Build();
//  }
//
//  std::shared_ptr<Transaction> CreateWalletTransaction()
//  {
//    std::ostringstream oss;
//    oss << "{ " << R"("amount": )" << 1000 << " }";
//
//    return TransactionBuilder()
//      .From(Address{prover_->identity()})
//      .TargetChainCode("fetch.token", fetch::BitVector{})
//      .Action("wealth")
//      .Data(oss.str())
//      .Signer(prover_->identity())
//      .Seal()
//      .Sign(*prover_)
//      .Build();
//  }
//
//  ProverPtr             prover_;
//  NetworkManagerPtr     network_manager_;
//  StorageUnitPtr        storage_;
//  ExecutorRpcServicePtr executor_service_;
//  ExecutorRpcClientPtr  executor_;
//  rng_type              rng_;
//
//  MuddlePtr muddle_;
//};
//
// TEST_F(ExecutorRpcTests, CheckDummyContract)
//{
//  EXPECT_CALL(*storage_, AddTransaction(_)).Times(1);
//  EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(1);
//
//  // create the dummy contract
//  auto tx = CreateDummyTransaction();
//
//  // store the transaction inside the store
//  storage_->AddTransaction(*tx);
//
//  fetch::BitVector shards{1};
//  shards.SetAllOne();
//
//  auto const result = executor_->Execute(tx->digest(), 0, 0, shards);
//  EXPECT_EQ(result.status, fetch::ledger::ExecutorInterface::Status::SUCCESS);
//}
//
// TEST_F(ExecutorRpcTests, CheckTokenContract)
//{
//  ::testing::InSequence seq;
//
//  EXPECT_CALL(*storage_, AddTransaction(_)).Times(1);
//  EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(1);
//  EXPECT_CALL(*storage_, Lock(_)).Times(1);
//  EXPECT_CALL(*storage_, Get(_)).Times(1);
//  EXPECT_CALL(*storage_, Set(_, _)).Times(1);
//  EXPECT_CALL(*storage_, Unlock(_)).Times(1);
//
//  // create the dummy contract
//  auto tx = CreateWalletTransaction();
//
//  // store the transaction inside the store
//  storage_->AddTransaction(*tx);
//
//  fetch::BitVector shards{1};
//  shards.SetAllOne();
//
//  auto const result = executor_->Execute(tx->digest(), 0, 0, shards);
//  EXPECT_EQ(result.status, fetch::ledger::ExecutorInterface::Status::SUCCESS);
//}
