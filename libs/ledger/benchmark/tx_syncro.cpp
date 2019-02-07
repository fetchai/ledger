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

// TODO(issue 399): Adjust test to work with current production code

//#include "core/byte_array/byte_array.hpp"
//#include "core/byte_array/const_byte_array.hpp"
//#include "core/random/lcg.hpp"
//#include "core/random/lfg.hpp"
//#include "crypto/ecdsa.hpp"
//#include "ledger/chain/mutable_transaction.hpp"
//#include "ledger/chain/transaction.hpp"
//#include "ledger/chain/transaction_serialization.hpp"
//#include "ledger/storage_unit/lane_connectivity_details.hpp"
//#include "ledger/storage_unit/lane_service.hpp"
//#include "network/service/server.hpp"
//#include "storage/object_store.hpp"
//#include "storage/object_store_protocol.hpp"
//#include "storage/object_store_syncronisation_protocol.hpp"
//
//#include <benchmark/benchmark.h>
//#include <vector>
//#include <algorithm>
//#include <gtest/gtest.h>
//#include <iostream>
//#include <utility>
//
// namespace {
//
// using fetch::byte_array::ConstByteArray;
// using fetch::byte_array::ByteArray;
// using fetch::storage::ResourceID;
// using fetch::ledger::VerifiedTransaction;
// using fetch::ledger::MutableTransaction;
// using fetch::crypto::ECDSASigner;
// using fetch::random::LinearCongruentialGenerator;
//
// using TransactionStore = fetch::ledger::LaneService::transaction_store_type;
// using TransactionList  = std::vector<VerifiedTransaction>;
//
// using namespace fetch::storage;
// using namespace fetch::byte_array;
// using namespace fetch::network;
// using namespace fetch::ledger;
// using namespace fetch::service;
// using namespace fetch::ledger;
//
// TransactionList GenerateTransactions(std::size_t count, bool large_packets)
//{
//  static constexpr std::size_t TX_SIZE      = 2048;
//  static constexpr std::size_t TX_WORD_SIZE = TX_SIZE / sizeof(uint64_t);
//
//  static_assert((TX_SIZE % sizeof(uint64_t)) == 0, "The transaction must be a multiple of
//  64bits");
//
//  LinearCongruentialGenerator rng;
//
//  TransactionList list;
//  list.reserve(count);
//
//  for (std::size_t i = 0; i < count; ++i)
//  {
//    MutableTransaction mtx;
//    mtx.set_contract_name("fetch.dummy");
//
//    if (large_packets)
//    {
//      ByteArray tx_data(TX_SIZE);
//      uint64_t *tx_data_raw = reinterpret_cast<uint64_t *>(tx_data.pointer());
//
//      for (std::size_t j = 0; j < TX_WORD_SIZE; ++j)
//      {
//        *tx_data_raw++ = rng();
//      }
//    }
//    else
//    {
//      mtx.set_data(std::to_string(i));
//    }
//
//    list.emplace_back(VerifiedTransaction::Create(std::move(mtx)));
//  }
//
//  return list;
//}
//
// class ControllerProtocol : public Protocol
//{
//
//  using connectivity_details_type  = LaneConnectivityDetails;
//  using service_client_type        = fetch::service::ServiceClient;
//  using shared_service_client_type = std::shared_ptr<service_client_type>;
//  using client_register_type       =
//  fetch::network::ConnectionRegister<connectivity_details_type>; using mutex_type =
//  fetch::mutex::Mutex; using connection_handle_type     =
//  client_register_type::connection_handle_type; using ClientRegister             =
//  ConnectionRegister<LaneConnectivityDetails>;
//
// public:
//  enum
//  {
//    CONNECT = 1
//  };
//
//  ControllerProtocol(ClientRegister reg, NetworkManager nm)
//    : register_{std::move(reg)}
//    , nm_{nm}
//  {
//    this->Expose(CONNECT, this, &ControllerProtocol::Connect);
//  }
//
//  void Connect(std::string const &host, uint16_t const &port)
//  {
//    std::shared_ptr<ServiceClient> client =
//        register_.CreateServiceClient<TCPClient>(nm_, host, port);
//
//    // Wait for connection to be open
//    if (!client->WaitForAlive(1000))
//    {
//      std::cout << "+++ Failed to connect client " << __LINE__ << std::endl;
//      exit(1);
//    }
//
//    {
//      std::lock_guard<mutex_type> lock_(services_mutex_);
//      services_[client->handle()] = client;
//    }
//  }
//
// private:
//  ClientRegister register_;
//  NetworkManager nm_;
//
//  mutex_type services_mutex_{__LINE__, __FILE__};
//  std::unordered_map<connection_handle_type, shared_service_client_type> services_;
//};
//
//// Convienience function to check a port is open
// void BlockUntilConnect(std::string host, uint16_t port)
//{
//  NetworkManager nm{1};
//  nm.Start();
//
//  while (true)
//  {
//    TCPClient client(nm);
//    client.Connect(host, port);
//
//    for (std::size_t i = 0; i < 10; ++i)
//    {
//      if (client.is_alive())
//      {
//        nm.Stop();
//        return;
//      }
//
//      std::this_thread::sleep_for(std::chrono::milliseconds(5));
//    }
//  }
//}
//
// void BlockUntilConnect(uint16_t port)
//{
//  BlockUntilConnect("localhost", port);
//}
//
//// Convenience function to call peer
// template <typename... Args>
// fetch::service::Promise CallPeer(NetworkManager nm, std::string address, uint16_t port,
//                                 Args &&... args)
//{
//  TCPClient client(nm);
//  client.Connect(address, port);
//
//  if (!client.WaitForAlive(2000))
//  {
//    std::cout << "Failed to connect to client at " << address << " " << port << std::endl;
//    exit(1);
//  }
//
//  ServiceClient s_client(client, nm);
//
//  fetch::service::Promise prom = s_client.Call(std::forward<Args>(args)...);
//
//  if (!prom->Wait(2000, true))
//  {
//    std::cout << "Failed to make call to client" << std::endl;
//    exit(1);
//  }
//
//  return prom;
//}
//
// class TestService : public ServiceServer<TCPServer>
//{
// public:
//  using ClientRegister   = ConnectionRegister<LaneConnectivityDetails>;
//  using TransactionStore = ObjectStore<VerifiedTransaction>;
//  using TxSyncProtocol   = ObjectStoreSyncronisationProtocol<ClientRegister, VerifiedTransaction>;
//  using TransactionStoreProtocol = ObjectStoreProtocol<VerifiedTransaction>;
//  using Super                    = ServiceServer<TCPServer>;
//
//  enum
//  {
//    TX_STORE = 1,
//    TX_STORE_SYNC,
//    CONTROLLER
//  };
//
//  TestService(uint16_t const &port, NetworkManager nm)
//    : Super(port, nm)
//  {
//    thread_pool_ = MakeThreadPool(1);
//    this->SetConnectionRegister(register_);
//
//    // Load/create TX store
//    std::string prefix = std::to_string(port);
//    tx_store_->New(prefix + "_tst_transaction.db", prefix + "_tst_transaction_index.db", true);
//
//    tx_sync_protocol_ =
//        std::make_unique<TxSyncProtocol>(TX_STORE_SYNC, register_, thread_pool_, tx_store_.get());
//
//    tx_store_protocol_ = std::make_unique<TransactionStoreProtocol>(tx_store_.get());
//    tx_store_protocol_->OnSetObject(
//        [this](VerifiedTransaction const &tx) { tx_sync_protocol_->AddToCache(tx); });
//
//    this->Add(TX_STORE, tx_store_protocol_.get());
//    this->Add(TX_STORE_SYNC, tx_sync_protocol_.get());
//
//    controller_protocol_ = std::make_unique<ControllerProtocol>(register_, nm);
//    this->Add(CONTROLLER, controller_protocol_.get());
//
//    thread_pool_->Start();
//    tx_sync_protocol_->Start();
//    this->Start();
//  }
//
//  ~TestService()
//  {
//    this->Stop();
//    thread_pool_->Stop();
//  }
//
// private:
//  ThreadPool     thread_pool_;
//  ClientRegister register_;
//
//  std::unique_ptr<TransactionStore>         tx_store_ = std::make_unique<TransactionStore>();
//  std::unique_ptr<TransactionStoreProtocol> tx_store_protocol_;
//  std::unique_ptr<TxSyncProtocol>           tx_sync_protocol_;
//
//  std::unique_ptr<ControllerProtocol> controller_protocol_;
//};
//
// void TxSync(benchmark::State &state)
//{
//
//  for (auto _ : state)
//  {
//    {
//      state.PauseTiming();
//      NetworkManager nm{50};
//      nm.Start();
//
//      std::this_thread::sleep_for(std::chrono::milliseconds(100));
//
//      uint16_t                                  initial_port       = 8000;
//      uint16_t                                  number_of_services = 5;
//      std::vector<std::shared_ptr<TestService>> services;
//
//      // Start up our services
//      for (uint16_t i = 0; i < number_of_services; ++i)
//      {
//        services.push_back(std::make_shared<TestService>(initial_port + i, nm));
//      }
//
//      // make sure they are all online
//      for (uint16_t i = 0; i < number_of_services; ++i)
//      {
//        BlockUntilConnect(uint16_t(initial_port + i));
//      }
//
//      // Connect our services to each other (chain config)
//      for (uint16_t i = 1; i < number_of_services; ++i)
//      {
//        CallPeer(nm, "localhost", uint16_t(initial_port + i), TestService::CONTROLLER,
//                 ControllerProtocol::CONNECT, ByteArray{"localhost"}, uint16_t(initial_port + i -
//                 1));
//      }
//
//      // Now send all the TX to one of the clients
//      auto transactions = GenerateTransactions(std::size_t(state.range(0)), true);
//
//      /*               resume timing              */
//      state.ResumeTiming();
//
//      for(auto const &tx : transactions)
//      {
//        CallPeer(nm, "localhost", uint16_t(initial_port), TestService::TX_STORE,
//                 ObjectStoreProtocol<VerifiedTransaction>::SET, ResourceID(tx.digest()), tx);
//      }
//
//      for (std::size_t i = 0; i < 10000; ++i)
//      {
//        // Check last node in chain until they have seen all TXs
//        auto promise = CallPeer(nm, "localhost", uint16_t(initial_port + number_of_services -1),
//                                TestService::TX_STORE_SYNC,
//                                TestService::TxSyncProtocol::SYNCED_COUNT);
//
//        uint64_t obj_count = promise->As<uint64_t>();
//
//        if(obj_count == transactions.size())
//        {
//          break;
//        }
//
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//
//        if(i == 10000-1)
//        {
//          std::cerr << "Failed to sync" << std::endl;
//          exit(1);
//        }
//      }
//
//      // Stop timing so that tear down not part of test
//      state.PauseTiming();
//      nm.Stop();
//    }
//
//    // resume timing after destruction of resources
//    state.ResumeTiming();
//  }
//}
//
//} // end namespace
//
// BENCHMARK(TxSync)->Range(8, 8<<10);
