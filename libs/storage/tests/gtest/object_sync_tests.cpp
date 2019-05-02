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

#include "core/random/lfg.hpp"
#include "core/service_ids.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "ledger/storage_unit/lane_controller.hpp"
#include "ledger/storage_unit/lane_controller_protocol.hpp"
#include "ledger/storage_unit/lane_service.hpp"
#include "ledger/storage_unit/transaction_store_sync_protocol.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/peer.hpp"
#include "storage/object_store_protocol.hpp"
#include "storage/transient_object_store.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <iostream>
#include <utility>

// Test of the object sync protocol in the style of the transaction sync in the lane service.
// A service, TestService here, owns an object store, and the protocols ensure that new objects
// (Using transactions here) will be synchronized with connected peers.

using namespace fetch::storage;
using namespace fetch::byte_array;
using namespace fetch::network;
using namespace fetch::ledger;
using namespace fetch::service;
using namespace fetch::ledger;
using namespace fetch;

using fetch::muddle::NetworkId;
using fetch::ledger::LaneService;

using LaneServicePtr = std::shared_ptr<LaneService>;

static constexpr char const *LOGGING_NAME = "ObjectSyncTest";

class MuddleTestClient
{
public:
  using Uri     = fetch::network::Uri;
  using Muddle  = fetch::muddle::Muddle;
  using Server  = fetch::muddle::rpc::Server;
  using Client  = fetch::muddle::rpc::Client;
  using Address = Muddle::Address;  // == a crypto::Identity.identifier_

  using MuddlePtr = std::shared_ptr<Muddle>;
  using ServerPtr = std::shared_ptr<Server>;
  using ClientPtr = std::shared_ptr<Client>;

  static std::shared_ptr<MuddleTestClient> CreateTestClient(NetworkManager &   tm,
                                                            const std::string &host, uint16_t port)
  {
    return CreateTestClient(tm, Uri(std::string("tcp://") + host + ":" + std::to_string(port)));
  }
  static std::shared_ptr<MuddleTestClient> CreateTestClient(NetworkManager &tm, const Uri &uri)
  {
    auto tc = std::make_shared<MuddleTestClient>();

    tc->muddle_ = Muddle::CreateMuddle(NetworkId{"Test"}, tm);
    tc->muddle_->Start({});

    tc->client_ = std::make_shared<Client>("Client", tc->muddle_->AsEndpoint(), Address(),
                                           SERVICE_LANE, CHANNEL_RPC);
    tc->muddle_->AddPeer(uri);

    int counter = 40;
    for (;;)
    {
      if (!counter--)
      {
        tc.reset();
        FETCH_LOG_WARN(LOGGING_NAME, "No peer, exiting..!");
        return tc;
      }
      if (tc->muddle_->UriToDirectAddress(uri, tc->address_))
      {
        return tc;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return tc;
  }

  template <typename... Args>
  Promise Call(fetch::service::protocol_handler_type const &protocol,
               fetch::service::function_handler_type const &function, Args &&... args)
  {
    return client_->CallSpecificAddress(address_, protocol, function, std::forward<Args>(args)...);
  }

  template <typename... Args>
  Promise CallAndWait(fetch::service::protocol_handler_type const &protocol,
                      fetch::service::function_handler_type const &function, Args &&... args)
  {
    auto prom = Call(protocol, function, std::forward<Args>(args)...);

    if (!prom->Wait(2000, true))
    {
      std::cout << "Failed to make call to client" << std::endl;
      exit(1);
    }
    return prom;
  }

  MuddleTestClient() = default;

  ~MuddleTestClient()
  {
    muddle_->Shutdown();
    muddle_->Stop();
  }

private:
  ClientPtr client_;
  Address   address_;
  MuddlePtr muddle_;
};

VerifiedTransaction GetRandomTx(crypto::ECDSASigner &certificate, uint64_t seed)
{
  MutableTransaction tx;
  // Fill the body of the TX with incrementing sequence so we can see it in wireshark etc.
  ByteArray marker;
  marker.Resize(5);

  for (std::size_t i = 0; i < 5; ++i)
  {
    marker[i] = uint8_t(i);
  }

  tx.set_fee(seed);  // easiest way to create random tx.
  tx.set_data(marker);
  tx.Sign(certificate.private_key());
  return VerifiedTransaction::Create(tx);
}

LaneServicePtr CreateLaneService(uint16_t start_port, NetworkManager const &nm, uint32_t lane = 0,
                                 uint32_t total_lanes = 1)
{
  using fetch::ledger::ShardConfig;
  using fetch::muddle::NetworkId;

  ShardConfig cfg;
  cfg.lane_id             = lane;
  cfg.num_lanes           = total_lanes;
  cfg.storage_path        = "object_sync_tests";
  cfg.external_identity   = std::make_shared<crypto::ECDSASigner>();
  cfg.external_port       = start_port;
  cfg.external_network_id = NetworkId{"EXT-"};
  cfg.internal_identity   = std::make_shared<crypto::ECDSASigner>();
  cfg.internal_port       = static_cast<uint16_t>(start_port + 1u);
  cfg.internal_network_id = NetworkId{"INT-"};

  return std::make_shared<LaneService>(nm, cfg, false, LaneService::Mode::CREATE_DATABASE);
}

// TODO(private issue 686): Reinstate object store tests
TEST(storage_object_store_sync_gtest, DISABLED_transaction_store_protocol_local_threads_1)
{
  NetworkManager nm{"NetMgr", 1};
  nm.Start();

  uint16_t                         initial_port = 8000;
  std::vector<VerifiedTransaction> sent;

  auto test_service = CreateLaneService(initial_port, nm);
  test_service->Start();

  auto                client = MuddleTestClient::CreateTestClient(nm, "127.0.0.1", initial_port);
  crypto::ECDSASigner certificate;

  FETCH_LOG_INFO(LOGGING_NAME, "Got client, sending tx");
  for (std::size_t i = 0; i < 100; ++i)
  {
    VerifiedTransaction tx = GetRandomTx(certificate, i);

    auto promise = client->CallAndWait(RPC_TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::SET,
                                       ResourceID(tx.digest()), tx);

    sent.push_back(tx);
  }
  FETCH_LOG_INFO(LOGGING_NAME, "Got client, sent all tx");

  // Now verify we can get the tx from the store
  for (auto const &tx : sent)
  {
    auto promise = client->CallAndWait(RPC_TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::GET,
                                       ResourceID(tx.digest()));

    uint64_t fee = promise->As<VerifiedTransaction>().summary().fee;

    if (fee != tx.summary().fee || tx.summary().fee == 0)
    {
      EXPECT_EQ(fee, tx.summary().fee);
    }
  }
  test_service->Stop();
  nm.Stop();
}

TEST(storage_object_store_sync_gtest, DISABLED_transaction_store_protocol_local_threads_50)
{
  NetworkManager nm{"NetMgr", 50};
  nm.Start();

  uint16_t                         initial_port = 9000;
  std::vector<VerifiedTransaction> sent;

  auto test_service = CreateLaneService(initial_port, nm);
  test_service->Start();

  auto                client = MuddleTestClient::CreateTestClient(nm, "localhost", initial_port);
  crypto::ECDSASigner certificate;
  for (std::size_t i = 0; i < 100; ++i)
  {
    VerifiedTransaction tx = GetRandomTx(certificate, i);

    auto promise = client->CallAndWait(RPC_TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::SET,
                                       ResourceID(tx.digest()), tx);

    sent.push_back(tx);
  }

  // Now verify we can get the tx from the store
  for (auto const &tx : sent)
  {
    auto promise = client->CallAndWait(RPC_TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::GET,
                                       ResourceID(tx.digest()));

    uint64_t fee = promise->As<VerifiedTransaction>().summary().fee;

    if (fee != tx.summary().fee || tx.summary().fee == 0)
    {
      EXPECT_EQ(fee, tx.summary().fee);
    }
  }
  test_service->Stop();
  nm.Stop();
}

TEST(storage_object_store_sync_gtest, DISABLED_transaction_store_protocol_local_threads_caching)
{
  // TODO(unknown): (HUT) : make this work with 1 - find the post blocking the NM.
  NetworkManager nm{"NetMgr", 50};
  nm.Start();

  uint16_t                    initial_port       = 10000;
  uint16_t                    number_of_services = 3;
  std::vector<LaneServicePtr> services;
  crypto::ECDSASigner         certificate;

  // Start up our services
  for (uint16_t i = 0; i < number_of_services; ++i)
  {
    services.push_back(CreateLaneService(static_cast<uint16_t>(initial_port + i), nm, i, 1));
    services.back()->Start();
  }

  FETCH_LOG_WARN(LOGGING_NAME, "Sending peers to clients");

  // Connect our services to each other
  for (uint16_t i = 0; i < number_of_services; ++i)
  {
    LaneController::UriSet uris;
    auto                   client = MuddleTestClient::CreateTestClient(nm, "localhost",
                                                     static_cast<uint16_t>(initial_port + i));
    for (uint16_t j = 0; j < number_of_services; ++j)
    {
      if (i != j)
      {
        uris.insert(
            LaneController::Uri(Peer("localhost", static_cast<uint16_t>(initial_port + j))));
      }
    }
    client->Call(RPC_CONTROLLER, LaneControllerProtocol::USE_THESE_PEERS, uris);
  }

  // Now send all the TX to one of the clients
  std::vector<VerifiedTransaction> sent;

  FETCH_LOG_WARN(LOGGING_NAME, "Sending txes to clients");

  auto client = MuddleTestClient::CreateTestClient(nm, "localhost", initial_port);

  for (std::size_t i = 0; i < 200; ++i)
  {
    VerifiedTransaction tx = GetRandomTx(certificate, i);

    auto promise = client->Call(RPC_TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::SET,
                                ResourceID(tx.digest()), tx);

    sent.push_back(tx);
  }

  FETCH_LOG_WARN(LOGGING_NAME, "Sent txes to client 1.");

  // Check all peers have identical transaction stores
  // bool failed_to_sync = false;

  // wait as long as is reasonable
  FETCH_LOG_WARN(LOGGING_NAME, "Waiting...");

  for (auto &service : services)
  {
    do
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    } while (!service->SyncIsReady());
  }

  client = nullptr;

  FETCH_LOG_WARN(LOGGING_NAME, "Verifying peers synced");

  // Now verify we can get the tx from the each client
  for (uint16_t i = 0; i < number_of_services; ++i)
  {
    client = MuddleTestClient::CreateTestClient(nm, "localhost",
                                                static_cast<uint16_t>(initial_port + i));
    for (auto const &tx : sent)
    {
      auto promise = client->CallAndWait(
          RPC_TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::GET, ResourceID(tx.digest()));

      VerifiedTransaction tx_rec = promise->As<VerifiedTransaction>();

      if (tx_rec.summary().fee != tx.summary().fee)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Client ", i, " ", ToHex(tx_rec.data()));
        EXPECT_EQ(tx_rec.summary().fee, tx.summary().fee);
      }
    }

    EXPECT_EQ(i, i);
  }
  client = nullptr;

  FETCH_LOG_INFO(LOGGING_NAME, "Test new joiner case");

  // Now test new joiner case, add new joiner
  services.push_back(CreateLaneService(static_cast<uint16_t>(initial_port + number_of_services), nm,
                                       number_of_services, 1));
  services.back()->Start();

  client = MuddleTestClient::CreateTestClient(
      nm, "localhost", static_cast<uint16_t>(initial_port + number_of_services));
  LaneController::UriSet uris;
  for (uint16_t j = 0; j < number_of_services; ++j)
  {
    uris.insert(LaneController::Uri(Peer("localhost", static_cast<uint16_t>(initial_port + j))));
  }
  client->Call(RPC_CONTROLLER, LaneControllerProtocol::USE_THESE_PEERS, uris);

  // Wait until the sync is done
  FETCH_LOG_INFO(LOGGING_NAME, "Waiting for new joiner to sync.");
  do
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  } while (!services.back()->SyncIsReady());

  FETCH_LOG_INFO(LOGGING_NAME, "Verifying new joiner sync.");

  bool failed_to_sync = false;

  // Verify the new joiner
  for (auto const &tx : sent)
  {
    auto promise = client->CallAndWait(RPC_TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::GET,
                                       ResourceID(tx.digest()));

    VerifiedTransaction tx_rec = promise->As<VerifiedTransaction>();

    if (tx_rec.summary().fee != tx.summary().fee)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Client ", number_of_services, " ", ToHex(tx_rec.data()));
      failed_to_sync = true;
    }
  }

  EXPECT_EQ(failed_to_sync, false);

  for (auto &service : services)
  {
    service->Stop();
  }
  services.clear();

  nm.Stop();
  FETCH_LOG_WARN(LOGGING_NAME, "End of test");
}

/*
SECTION("Test transaction store sync protocol (caching, then new joiner, threads 1) ")
{
  // TODO: (HUT) : make this work with 1 - find the post blocking the NM.
  NetworkManager nm{1};
  nm.Start();

  uint16_t                                  initial_port       = 8080;
  uint16_t                                  number_of_services = 5;
  std::vector<std::shared_ptr<TestService>> services;

  // Start up our services
  for (uint16_t i = 0; i < number_of_services; ++i)
  {
    services.push_back(std::make_shared<TestService>(initial_port + i, nm));
  }

  // make sure they are all online
  for (uint16_t i = 0; i < number_of_services; ++i)
  {
    BlockUntilConnect(initial_port+i);
  }

  // Connect our services to each other
  for (uint16_t i = 0; i < number_of_services; ++i)
  {
    for (uint16_t j = 0; j < number_of_services; ++j)
    {
      if (i != j)
      {
        CallPeer(nm, "localhost", uint16_t(initial_port + i),
                       TestService::CONTROLLER, ControllerProtocol::CONNECT,
                       ByteArray{"localhost"}, uint16_t(initial_port + j));
      }
    }
  }

  // Now send all the TX to one of the clients
  std::vector<VerifiedTransaction> sent;

  std::cout << "Sending txes to clients" << std::endl;

  for (std::size_t i = 0; i < 500; ++i)
  {
    VerifiedTransaction tx = GetRandomTx(i);

    CallPeer(nm, "localhost", uint16_t(initial_port),
                     TestService::TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::SET,
                     ResourceID(tx.digest()), tx);

    sent.push_back(tx);
  }

  // Check all peers have identical transaction stores
  bool failed_to_sync = false;

  // wait as long as is reasonable
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  std::cout << "Verifying peers synced" << std::endl;

  // Now verify we can get the tx from the each client
  for (uint16_t i = 0; i < number_of_services; ++i)
  {
    for (auto const &tx : sent)
    {
      auto promise = CallPeer(nm, "localhost", uint16_t(initial_port + i),
                       TestService::TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::GET,
                       ResourceID(tx.digest()));


      VerifiedTransaction tx_rec = promise.As<VerifiedTransaction>();

      if (tx_rec.summary().fee != tx.summary().fee)
      {
        std::cout << "Client " << i << std::endl;
        std::cout << ToHex(tx_rec.data()) << std::endl;
        EXPECT(tx_rec.summary().fee == tx.summary().fee);
      }
    }

    EXPECT(i == i);
  }

  std::cout << "Test new joiner case" << std::endl;

  // Now test new joiner case, add new joiner
  services.push_back(std::make_shared<TestService>(initial_port + number_of_services, nm));

  BlockUntilConnect(initial_port + number_of_services);

  // Connect to peers
  for (uint16_t i = 0; i < number_of_services; ++i)
  {
    CallPeer(nm, "localhost", uint16_t(initial_port + number_of_services),
             TestService::CONTROLLER, ControllerProtocol::CONNECT,
             ByteArray{"localhost"}, uint16_t(initial_port + i));
  }

  // Wait until the sync is done
  while(true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto promise = CallPeer(nm, "localhost", uint16_t(initial_port + number_of_services),
                        TestService::TX_STORE_SYNC, TestService::TxSyncProtocol::FINISHED_SYNC);

    if(promise.As<bool>())
    {
      break;
    }

    std::cout << "Waiting for new joiner to sync." << std::endl;
  }

  std::cout << "Verifying new joiner sync." << std::endl;

  failed_to_sync = false;

  // Verify the new joiner
  for (auto const &tx : sent)
  {
    auto promise = CallPeer(nm, "localhost", uint16_t(initial_port + number_of_services),
                        TestService::TX_STORE, ObjectStoreProtocol<VerifiedTransaction>::GET,
                        ResourceID(tx.digest()));


    if (promise.As<VerifiedTransaction>().summary().fee != tx.summary().fee)
    {
      failed_to_sync = true;
      std::cout << "Expecting: " << tx.summary().fee << std::endl;
      EXPECT("Fees are the same " == "x");
    }
  }

  EXPECT(failed_to_sync == false);

  nm.Stop();
}; */
