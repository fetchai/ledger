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

#include "storage/object_store.hpp"
#include "core/random/lfg.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "network/service/server.hpp"
#include "storage/object_store_syncronisation_protocol.hpp"
#include "storage/object_store_protocol.hpp"
#include "testing/unittest.hpp"
#include <algorithm>
#include <iostream>

using namespace fetch::storage;
using namespace fetch::byte_array;
using namespace fetch::network;
using namespace fetch::chain;
using namespace fetch::service;
using namespace fetch::ledger;

VerifiedTransaction GetVerifiedTx(uint64_t seed)
{
  MutableTransaction tx;
  tx.set_fee(seed); // easiest way to create random tx.
  return VerifiedTransaction::Create(tx);
}

class ControllerProtocol : public Protocol
{

using connectivity_details_type  = LaneConnectivityDetails;
using service_client_type        = fetch::service::ServiceClient;
using shared_service_client_type = std::shared_ptr<service_client_type>;
using client_register_type       = fetch::network::ConnectionRegister<connectivity_details_type>;
using mutex_type                 = fetch::mutex::Mutex;
using connection_handle_type     = client_register_type::connection_handle_type;
using ClientRegister             = ConnectionRegister<LaneConnectivityDetails>;

public:

  enum
  {
    CONNECT = 1
  };

  ControllerProtocol(ClientRegister reg, NetworkManager nm)
    : register_{reg}
    , nm_{nm}
  {
    this->Expose(CONNECT, this, &ControllerProtocol::Connect);
  }

  void Connect(std::string const &host, uint16_t const &port)
  {

    std::shared_ptr<ServiceClient> client =
        register_.CreateServiceClient<TCPClient>(nm_, host, port);

    // Wait for connection to be open
    if(!client->WaitForAlive(500))
    {
      std::cout << "Failed to connect client " << __LINE__ << std::endl;
      exit(1);
    }

    {
      std::lock_guard<mutex_type> lock_(services_mutex_);
      services_[client->handle()] = client;
    }

    // Setting up details such that the rest of the lane what kind of
    // connection we are dealing with.
    auto details = register_.GetDetails(client->handle());

    details->is_outgoing = true;
    details->is_peer     = true;
  }

private:
  ClientRegister register_;
  NetworkManager nm_;

  // TODO: (HUT) : determine what this is for
  mutex_type                                                             services_mutex_;
  std::unordered_map<connection_handle_type, shared_service_client_type> services_;
};

class TestService : public ServiceServer<TCPServer>
{
public:
  using ClientRegister           = ConnectionRegister<LaneConnectivityDetails>;
  using TransactionStore         = ObjectStore<VerifiedTransaction>;
  using TxSyncProtocol           =
    ObjectStoreSyncronisationProtocol<ClientRegister, VerifiedTransaction>;
  using TransactionStoreProtocol = ObjectStoreProtocol<VerifiedTransaction>;
  using SuperType                = ServiceServer<TCPServer>;

  enum
  {
    IDENTITY = 1,
    TX_STORE,
    TX_STORE_SYNC,
    CONTROLLER
  };

  TestService(uint16_t const &port, NetworkManager nm)
    : SuperType(port, nm)
  {
    thread_pool_ = MakeThreadPool(1);
    this->SetConnectionRegister(register_);

    // Load TX store
    std::string prefix = std::to_string(port);
    tx_store_->New(prefix + "_tst_transaction.db", prefix + "_tst_transaction_index.db", true);

    tx_sync_protocol_  = std::make_unique<TxSyncProtocol>(TX_STORE_SYNC, register_,
                                                          thread_pool_, tx_store_.get());

    tx_store_protocol_ = std::make_unique<TransactionStoreProtocol>(tx_store_.get());
    tx_store_protocol_->OnSetObject(
        [this](VerifiedTransaction const &tx) { tx_sync_protocol_->AddToCache(tx); });

    this->Add(TX_STORE, tx_store_protocol_.get());
    this->Add(TX_STORE_SYNC, tx_sync_protocol_.get());

    controller_protocol_ = std::make_unique<ControllerProtocol>(register_, nm);
    this->Add(CONTROLLER, controller_protocol_.get());

    thread_pool_->Start();
    tx_sync_protocol_->Start();
  }

  ~TestService()
  {
  }

  ThreadPool                                thread_pool_;
  ClientRegister                            register_;

  std::unique_ptr<TransactionStore>         tx_store_ =    std::make_unique<TransactionStore>();
  std::unique_ptr<TransactionStoreProtocol> tx_store_protocol_;
  std::unique_ptr<TxSyncProtocol>           tx_sync_protocol_;

  std::unique_ptr<ControllerProtocol> controller_protocol_;

};

int main(int argc, char const **argv)
{
  SCENARIO("Testing object store sync")
  {
    SECTION("Test transaction store protocol (local) ")
    {
      NetworkManager nm{10};
      nm.Start();

      uint16_t initial_port = 8080;
      std::vector<VerifiedTransaction> sent;

      TestService test_service(initial_port, nm);

      for (std::size_t i = 0; i < 100; ++i)
      {
        VerifiedTransaction tx = GetVerifiedTx(i);

        TCPClient client(nm);
        client.Connect("localhost", initial_port);

        for (std::size_t i = 0;; ++i)
        {
          if(client.is_alive())
          {
            break;
          }

          if(i == 100)
          {
            std::cout << "Failed to connect to server" << std::endl;
            EXPECT(client.is_alive() == true);
            exit(1);
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        ServiceClient s_client(client, nm);

        auto promise = s_client.Call(TestService::TX_STORE,
                                     ObjectStoreProtocol<VerifiedTransaction>::SET,
                                     ResourceID(tx.digest()), tx);

        promise.Wait(1000);
        sent.push_back(tx);
      }

      // Now verify we can get the tx from the store
      for(auto const &tx: sent)
      {
        TCPClient client(nm);
        client.Connect("localhost", initial_port);

        for (std::size_t i = 0;; ++i)
        {
          if(client.is_alive())
          {
            break;
          }

          if(i == 100)
          {
            std::cout << "Failed to connect to server" << std::endl;
            EXPECT(client.is_alive() == true);
            exit(1);
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        ServiceClient s_client(client, nm);

        auto promise = s_client.Call(TestService::TX_STORE,
                                     ObjectStoreProtocol<VerifiedTransaction>::GET,
                                     ResourceID(tx.digest()));

        EXPECT(promise.As<VerifiedTransaction>().summary().fee == tx.summary().fee);
      }

      nm.Stop();
    };

    SECTION("Test transaction store sync protocol (caching, then new joiner) ")
    {
      NetworkManager nm{40};
      nm.Start();

      uint16_t initial_port = 8080;
      uint16_t number_of_services = 5;
      std::vector<std::shared_ptr<TestService>> services;

      // Start up our services
      for (uint16_t i = 0; i < number_of_services; ++i)
      {
        services.push_back(std::make_shared<TestService>(initial_port+i, nm));
      }

      // Connect our services to each other
      for (uint16_t i = 0; i < number_of_services; ++i)
      {
        for (uint16_t j = 0; j < number_of_services; ++j)
        {
          if(i != j)
          {

            TCPClient client(nm);
            client.Connect("localhost", initial_port+i); // Connect to i

            if(!client.WaitForAlive(500))
            {
              std::cout << "Failed to connect client " << __LINE__ << std::endl;
              exit(1);
            }

            ServiceClient s_client(client, nm);

            auto promise = s_client.Call(TestService::CONTROLLER,
                                         ControllerProtocol::CONNECT,
                                         ByteArray{"localhost"},
                                         uint16_t(initial_port+j));

            //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            //std::cout << "Waiting" << std::endl;

            if(!promise.Wait(500))
            {
              std::cout << "Fail" << std::endl;
              exit(1);
            }

            //std::cout << "win" << std::endl;
          }
        }
      }

      std::cout << "Successfully connected peers together" << std::endl;
      //std::this_thread::sleep_for(std::chrono::milliseconds(5000));

      std::cout << "Testing sync." << std::endl;

      // Now send all the TX to one of the clients
      std::vector<VerifiedTransaction> sent;

      for (std::size_t i = 0; i < 500; ++i)
      {
        VerifiedTransaction tx = GetVerifiedTx(i);

        TCPClient client(nm);
        client.Connect("localhost", initial_port);

        for (std::size_t i = 0;; ++i)
        {
          if(client.is_alive())
          {
            break;
          }

          if(i == 100)
          {
            std::cout << "Failed to connect to server" << std::endl;
            EXPECT(client.is_alive() == true);
            exit(1);
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        ServiceClient s_client(client, nm);

        auto promise = s_client.Call(TestService::TX_STORE,
                                     ObjectStoreProtocol<VerifiedTransaction>::SET,
                                     ResourceID(tx.digest()), tx);

        if(!promise.Wait(1000))
        {
          std::cout << "Failed to make call!" << std::endl;
        }

        sent.push_back(tx);
      }

      bool failed_to_sync = false;

      // Now verify we can get the tx from the each client
      for (uint16_t i = 0; i < number_of_services; ++i)
      {
        for(auto const &tx: sent)
        {
          TCPClient client(nm);
          client.Connect("localhost", initial_port+i);

          for (std::size_t i = 0;; ++i)
          {
            if(client.is_alive())
            {
              break;
            }

            if(i == 500)
            {
              std::cout << "Failed to connect to server" << std::endl;
              EXPECT(client.is_alive() == true);
              exit(1);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
          }

          ServiceClient s_client(client, nm);

          auto promise = s_client.Call(TestService::TX_STORE,
                                       ObjectStoreProtocol<VerifiedTransaction>::GET,
                                       ResourceID(tx.digest()));

          if(promise.As<VerifiedTransaction>().summary().fee != tx.summary().fee)
          {
            failed_to_sync = true;
            EXPECT("Fees are the same " == "x");
          }
        }
      }

      EXPECT(failed_to_sync == false);

      // Now test new joiner case
      services.push_back(std::make_shared<TestService>(initial_port+number_of_services, nm));

      for (uint16_t i = 0; i < number_of_services; ++i)
      {
        // Connect to newest peer
        TCPClient client(nm);
        client.Connect("localhost", uint16_t(initial_port+number_of_services));

        if(!client.WaitForAlive(500))
        {
          std::cout << "Failed to connect client " << __LINE__ << std::endl;
          exit(1);
        }

        ServiceClient s_client(client, nm);

        // Make peer connect to peer 'i'
        auto promise = s_client.Call(TestService::CONTROLLER,
                                     ControllerProtocol::CONNECT,
                                     ByteArray{"localhost"},
                                     uint16_t(initial_port+i));

        if(!promise.Wait(500))
        {
          std::cout << "Fail" << std::endl;
          exit(1);
        }
      }

      {
        TCPClient client(nm);
        client.Connect("localhost", initial_port+number_of_services);

        if(!client.WaitForAlive(500))
        {
          std::cout << "Failed to connect client " << __LINE__ << std::endl;
          exit(1);
        }

        ServiceClient s_client(client, nm);

        auto promise = s_client.Call(TestService::TX_STORE_SYNC,
                                     TestService::TxSyncProtocol::START_SYNC);

        if(!promise.Wait(500))
        {
          std::cout << "Fail" << std::endl;
          exit(1);
        }
      }

      // Wait until the sync is done
      {
        TCPClient client(nm);
        client.Connect("localhost", initial_port+number_of_services);

        for (std::size_t i = 0;; ++i)
        {
          if(client.is_alive())
          {
            break;
          }

          if(i == 500)
          {
            std::cout << "Failed to connect to server" << std::endl;
            EXPECT(client.is_alive() == true);
            exit(1);
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        ServiceClient s_client(client, nm);

        // Wait for sync
        for (std::size_t i = 0; ; ++i)
        {
          bool result = s_client.Call(TestService::TX_STORE_SYNC,
                                      TestService::TxSyncProtocol::FINISHED_SYNC);

          if(result)
          {
            break;
          }

          if(i == 1000)
          {
            std::cout << "sync timed out" << std::endl;
            exit(1);
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      }


      std::cout << "Verifying" << std::endl;
      failed_to_sync = false;
      // Verify the new joiner
      for(auto const &tx: sent)
      {
        TCPClient client(nm);
        client.Connect("localhost", initial_port+number_of_services);

        for (std::size_t i = 0;; ++i)
        {
          if(client.is_alive())
          {
            break;
          }

          if(i == 500)
          {
            std::cout << "Failed to connect to server" << std::endl;
            EXPECT(client.is_alive() == true);
            exit(1);
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        ServiceClient s_client(client, nm);

        auto promise = s_client.Call(TestService::TX_STORE,
                                     ObjectStoreProtocol<VerifiedTransaction>::GET,
                                     ResourceID(tx.digest()));

        if(promise.As<VerifiedTransaction>().summary().fee != tx.summary().fee)
        {
          failed_to_sync = true;
          EXPECT("Fees are the same " == "x" && tx.summary().fee == tx.summary().fee);
        }
      }

      EXPECT(failed_to_sync == false);

      nm.Stop();
    };
  };

  return 0;
}
