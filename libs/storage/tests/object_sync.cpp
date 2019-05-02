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

//#include "core/random/lfg.hpp"
//#include "ledger/chain/mutable_transaction.hpp"
//#include "ledger/chain/transaction.hpp"
//#include "ledger/chain/transaction_serialization.hpp"
//#include "ledger/storage_unit/lane_connectivity_details.hpp"
//#include "network/service/server.hpp"
//#include "storage/object_store.hpp"
//#include "storage/object_store_protocol.hpp"
//#include "storage/object_store_syncronisation_protocol.hpp"
//#include "testing/unittest.hpp"
//#include <algorithm>
//#include <iostream>
//#include <utility>
//
//// Test of the object sync protocol in the style of the transaction sync in the lane service.
//// A service, TestService here, owns an object store, and the protocols ensure that new objects
//// (Using transactions here) will be synchronized with connected peers.
//
//using namespace fetch::storage;
//using namespace fetch::byte_array;
//using namespace fetch::network;
//using namespace fetch::chain;
//using namespace fetch::service;
//using namespace fetch::ledger;
//
//template <typename... Args>
//fetch::service::Promise CallPeer(NetworkManager nm, std::string address, uint16_t port,
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
//void BlockUntilConnect(std::string host, uint16_t port)
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
//void BlockUntilConnect(uint16_t port)
//{
//  BlockUntilConnect("localhost", port);
//}
//
//VerifiedTransaction GetRandomTx(uint64_t seed)
//{
//  MutableTransaction tx;
//  // Fill the body of the TX with incrementing sequence so we can see it in wireshark etc.
//  ByteArray marker;
//  marker.Resize(5);
//
//  for (std::size_t i = 0; i < 5; ++i)
//  {
//    marker[i] = uint8_t(i);
//  }
//
//  tx.set_fee(seed);  // easiest way to create random tx.
//  tx.set_data(marker);
//  return VerifiedTransaction::Create(tx);
//}
//
//class ControllerProtocol : public Protocol
//{
//
//  using connectivity_details_type  = LaneConnectivityDetails;
//  using service_client_type        = fetch::service::ServiceClient;
//  using shared_service_client_type = std::shared_ptr<service_client_type>;
//  using client_register_type       = fetch::network::ConnectionRegister<connectivity_details_type>;
//  using mutex_type                 = fetch::mutex::Mutex;
//  using connection_handle_type     = client_register_type::connection_handle_type;
//  using ClientRegister             = ConnectionRegister<LaneConnectivityDetails>;
//
//public:
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
//private:
//  ClientRegister register_;
//  NetworkManager nm_;
//
//  mutex_type services_mutex_{__LINE__, __FILE__};
//  std::unordered_map<connection_handle_type, shared_service_client_type> services_;
//};
//
//class TestService : public ServiceServer<TCPServer>
//{
//public:
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
//    // Load TX store
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
//
//    this->Start();
//  }
//
//  ~TestService()
//  {
//    this->Stop();
//    thread_pool_->Stop();
//  }
//
//private:
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
//int main(int argc, char const **argv)
//{
//  SCENARIO("Testing object store sync")
//  {
//    SECTION("Test transaction store protocol local, threads = 1")
//    {
//      NetworkManager nm{1};
//      nm.Start();
//
//      uint16_t                         initial_port = 8080;
//      std::vector<VerifiedTransaction> sent;
//
//      TestService test_service(initial_port, nm);
//
//      BlockUntilConnect("localhost", initial_port);
//
//      for (std::size_t i = 0; i < 100; ++i)
//      {
//        VerifiedTransaction tx = GetRandomTx(i);
//
//        auto promise =
//            CallPeer(nm, "localhost", initial_port, TestService::TX_STORE,
//                     ObjectStoreProtocol<VerifiedTransaction>::SET, ResourceID(tx.digest()), tx);
//
//        sent.push_back(tx);
//      }
//
//      // Now verify we can get the tx from the store
//      for (auto const &tx : sent)
//      {
//        auto promise =
//            CallPeer(nm, "localhost", initial_port, TestService::TX_STORE,
//                     ObjectStoreProtocol<VerifiedTransaction>::GET, ResourceID(tx.digest()));
//
//        uint64_t fee = promise->As<VerifiedTransaction>().summary().fee;
//
//        if (fee != tx.summary().fee || tx.summary().fee == 0)
//        {
//          EXPECT(fee == tx.summary().fee);
//        }
//      }
//
//      nm.Stop();
//    };
//
//    SECTION("Test transaction store protocol local, threads = 50")
//    {
//      NetworkManager nm{50};
//      nm.Start();
//
//      uint16_t                         initial_port = 8080;
//      std::vector<VerifiedTransaction> sent;
//
//      TestService test_service(initial_port, nm);
//
//      BlockUntilConnect("localhost", initial_port);
//
//      for (std::size_t i = 0; i < 100; ++i)
//      {
//        VerifiedTransaction tx = GetRandomTx(i);
//
//        auto promise =
//            CallPeer(nm, "localhost", initial_port, TestService::TX_STORE,
//                     ObjectStoreProtocol<VerifiedTransaction>::SET, ResourceID(tx.digest()), tx);
//
//        sent.push_back(tx);
//      }
//
//      // Now verify we can get the tx from the store
//      for (auto const &tx : sent)
//      {
//        auto promise =
//            CallPeer(nm, "localhost", initial_port, TestService::TX_STORE,
//                     ObjectStoreProtocol<VerifiedTransaction>::GET, ResourceID(tx.digest()));
//
//        uint64_t fee = promise->As<VerifiedTransaction>().summary().fee;
//
//        if (fee != tx.summary().fee || tx.summary().fee == 0)
//        {
//          EXPECT(fee == tx.summary().fee);
//        }
//      }
//
//      nm.Stop();
//    };
//
//    SECTION("Test transaction store sync protocol (caching, then new joiner, threads 50) ")
//    {
//      // TODO(unknown): (HUT) : make this work with 1 - find the post blocking the NM.
//      NetworkManager nm{50};
//      nm.Start();
//
//      uint16_t                                  initial_port       = 8080;
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
//      // Connect our services to each other
//      for (uint16_t i = 0; i < number_of_services; ++i)
//      {
//        for (uint16_t j = 0; j < number_of_services; ++j)
//        {
//          if (i != j)
//          {
//            CallPeer(nm, "localhost", uint16_t(initial_port + i), TestService::CONTROLLER,
//                     ControllerProtocol::CONNECT, ByteArray{"localhost"},
//                     uint16_t(initial_port + j));
//          }
//        }
//      }
//
//      // Now send all the TX to one of the clients
//      std::vector<VerifiedTransaction> sent;
//
//      std::cout << "Sending txes to clients" << std::endl;
//
//      for (std::size_t i = 0; i < 500; ++i)
//      {
//        VerifiedTransaction tx = GetRandomTx(i);
//
//        CallPeer(nm, "localhost", uint16_t(initial_port), TestService::TX_STORE,
//                 ObjectStoreProtocol<VerifiedTransaction>::SET, ResourceID(tx.digest()), tx);
//
//        sent.push_back(tx);
//      }
//
//      // Check all peers have identical transaction stores
//      bool failed_to_sync = false;
//
//      // wait as long as is reasonable
//      std::this_thread::sleep_for(std::chrono::milliseconds(3000));
//
//      std::cout << "Verifying peers synced" << std::endl;
//
//      // Now verify we can get the tx from the each client
//      for (uint16_t i = 0; i < number_of_services; ++i)
//      {
//        for (auto const &tx : sent)
//        {
//          auto promise =
//              CallPeer(nm, "localhost", uint16_t(initial_port + i), TestService::TX_STORE,
//                       ObjectStoreProtocol<VerifiedTransaction>::GET, ResourceID(tx.digest()));
//
//          VerifiedTransaction tx_rec = promise->As<VerifiedTransaction>();
//
//          if (tx_rec.summary().fee != tx.summary().fee)
//          {
//            std::cout << "Client " << i << std::endl;
//            std::cout << ToHex(tx_rec.data()) << std::endl;
//            EXPECT(tx_rec.summary().fee == tx.summary().fee);
//          }
//        }
//
//        EXPECT(i == i);
//      }
//
//      std::cout << "Test new joiner case" << std::endl;
//
//      // Now test new joiner case, add new joiner
//      services.push_back(
//          std::make_shared<TestService>(uint16_t(initial_port + number_of_services), nm));
//
//      BlockUntilConnect(uint16_t(initial_port + number_of_services));
//
//      // Connect to peers
//      for (uint16_t i = 0; i < number_of_services; ++i)
//      {
//        CallPeer(nm, "localhost", uint16_t(initial_port + number_of_services),
//                 TestService::CONTROLLER, ControllerProtocol::CONNECT, ByteArray{"localhost"},
//                 uint16_t(initial_port + i));
//      }
//
//      // Wait until the sync is done
//      while (true)
//      {
//        std::this_thread::sleep_for(std::chrono::milliseconds(100));
//
//        auto promise =
//            CallPeer(nm, "localhost", uint16_t(initial_port + number_of_services),
//                     TestService::TX_STORE_SYNC, TestService::TxSyncProtocol::FINISHED_SYNC);
//
//        if (promise->As<bool>())
//        {
//          break;
//        }
//
//        std::cout << "Waiting for new joiner to sync." << std::endl;
//      }
//
//      std::cout << "Verifying new joiner sync." << std::endl;
//
//      failed_to_sync = false;
//
//      // Verify the new joiner
//      for (auto const &tx : sent)
//      {
//        auto promise = CallPeer(
//            nm, "localhost", uint16_t(initial_port + number_of_services), TestService::TX_STORE,
//            ObjectStoreProtocol<VerifiedTransaction>::GET, ResourceID(tx.digest()));
//
//        if (promise->As<VerifiedTransaction>().summary().fee != tx.summary().fee)
//        {
//          failed_to_sync = true;
//          std::cout << "Expecting: " << tx.summary().fee << std::endl;
//          EXPECT(std::string("Fees are the same ") == std::string("x"));
//        }
//      }
//
//      EXPECT(failed_to_sync == false);
//
//      nm.Stop();
//    };
//
//    /*
//    SECTION("Test transaction store sync protocol (caching, then new joiner, threads 1) ")
//    {
//      // TODO: (HUT) : make this work with 1 - find the post blocking the NM.
//      NetworkManager nm{1};
//      nm.Start();
//
//      uint16_t                                  initial_port       = 8080;
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
//        BlockUntilConnect(initial_port+i);
//      }
//
//      // Connect our services to each other
//      for (uint16_t i = 0; i < number_of_services; ++i)
//      {
//        for (uint16_t j = 0; j < number_of_services; ++j)
//        {
//          if (i != j)
//          {
//            CallPeer(nm, "localhost", uint16_t(initial_port + i),
//                           TestService::CONTROLLER, ControllerProtocol::CONNECT,
//                           ByteArray{"localhost"}, uint16_t(initial_port + j));
//          }
//        }
//      }
//
//      // Now send all the TX to one of the clients
//      std::vector<VerifiedTransaction> sent;
//
//      std::cout << "Sending txes to clients" << std::endl;
//
//      for (std::size_t i = 0; i < 500; ++i)
//      {
//        VerifiedTransaction tx = GetRandomTx(i);
//
//        CallPeer(nm, "localhost", uint16_t(initial_port),
//                         TestService::TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::SET,
//                         ResourceID(tx.digest()), tx);
//
//        sent.push_back(tx);
//      }
//
//      // Check all peers have identical transaction stores
//      bool failed_to_sync = false;
//
//      // wait as long as is reasonable
//      std::this_thread::sleep_for(std::chrono::milliseconds(3000));
//
//      std::cout << "Verifying peers synced" << std::endl;
//
//      // Now verify we can get the tx from the each client
//      for (uint16_t i = 0; i < number_of_services; ++i)
//      {
//        for (auto const &tx : sent)
//        {
//          auto promise = CallPeer(nm, "localhost", uint16_t(initial_port + i),
//                           TestService::TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::GET,
//                           ResourceID(tx.digest()));
//
//
//          VerifiedTransaction tx_rec = promise.As<VerifiedTransaction>();
//
//          if (tx_rec.summary().fee != tx.summary().fee)
//          {
//            std::cout << "Client " << i << std::endl;
//            std::cout << ToHex(tx_rec.data()) << std::endl;
//            EXPECT(tx_rec.summary().fee == tx.summary().fee);
//          }
//        }
//
//        EXPECT(i == i);
//      }
//
//      std::cout << "Test new joiner case" << std::endl;
//
//      // Now test new joiner case, add new joiner
//      services.push_back(std::make_shared<TestService>(initial_port + number_of_services, nm));
//
//      BlockUntilConnect(initial_port + number_of_services);
//
//      // Connect to peers
//      for (uint16_t i = 0; i < number_of_services; ++i)
//      {
//        CallPeer(nm, "localhost", uint16_t(initial_port + number_of_services),
//                 TestService::CONTROLLER, ControllerProtocol::CONNECT,
//                 ByteArray{"localhost"}, uint16_t(initial_port + i));
//      }
//
//      // Wait until the sync is done
//      while(true)
//      {
//        std::this_thread::sleep_for(std::chrono::milliseconds(100));
//
//        auto promise = CallPeer(nm, "localhost", uint16_t(initial_port + number_of_services),
//                            TestService::TX_STORE_SYNC, TestService::TxSyncProtocol::FINISHED_SYNC);
//
//        if(promise.As<bool>())
//        {
//          break;
//        }
//
//        std::cout << "Waiting for new joiner to sync." << std::endl;
//      }
//
//      std::cout << "Verifying new joiner sync." << std::endl;
//
//      failed_to_sync = false;
//
//      // Verify the new joiner
//      for (auto const &tx : sent)
//      {
//        auto promise = CallPeer(nm, "localhost", uint16_t(initial_port + number_of_services),
//                            TestService::TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::GET,
//                            ResourceID(tx.digest()));
//
//
//        if (promise.As<VerifiedTransaction>().summary().fee != tx.summary().fee)
//        {
//          failed_to_sync = true;
//          std::cout << "Expecting: " << tx.summary().fee << std::endl;
//          EXPECT("Fees are the same " == "x");
//        }
//      }
//
//      EXPECT(failed_to_sync == false);
//
//      nm.Stop();
//    }; */
//  };
//
//  return 0;
//}
