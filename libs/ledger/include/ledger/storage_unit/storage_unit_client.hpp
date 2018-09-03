#pragma once
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

#include "core/logger.hpp"
#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "ledger/storage_unit/lane_identity.hpp"
#include "ledger/storage_unit/lane_identity_protocol.hpp"
#include "ledger/storage_unit/lane_service.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "network/management/connection_register.hpp"
#include "network/service/client.hpp"

#include "ledger/chain/transaction.hpp"
#include "storage/document_store_protocol.hpp"
#include "storage/object_store_protocol.hpp"

#include <chrono>
#include <thread>
#include <utility>

namespace fetch {
namespace ledger {

class StorageUnitClient : public StorageUnitInterface
{
public:
  struct ClientDetails
  {
    crypto::Identity      identity;
    std::atomic<uint32_t> lane;
  };

  using service_client_type        = service::ServiceClient;
  using shared_service_client_type = std::shared_ptr<service_client_type>;
  using client_register_type       = fetch::network::ConnectionRegister<ClientDetails>;
  using connection_handle_type     = client_register_type::connection_handle_type;
  using network_manager_type       = fetch::network::NetworkManager;
  using lane_type                  = LaneIdentity::lane_type;

  static constexpr char const *LOGGING_NAME = "StorageUnitClient";

  explicit StorageUnitClient(network_manager_type const &tm) : network_manager_(tm)
  {
    id_ = "my-fetch-id";
  }

  StorageUnitClient(StorageUnitClient const &) = default;
  StorageUnitClient(StorageUnitClient &&)      = default;

  void SetNumberOfLanes(lane_type const &count)
  {
    lanes_.resize(count);
    SetLaneLog2(uint32_t(lanes_.size()));
    assert(count == (1u << log2_lanes_));
  }

  template <typename T>
  crypto::Identity AddLaneConnection(byte_array::ByteArray const &host, uint16_t const &port)
  {
    shared_service_client_type client =
        register_.template CreateServiceClient<T>(network_manager_, host, port);

    // Waiting for connection to be open
    bool connection_timeout = false;
    for (std::size_t n = 0; n < 10; ++n)
    {

      // ensure the connection is live
      if (client->is_alive())
      {

        // make the client call
        auto p = client->Call(RPC_IDENTITY, LaneIdentityProtocol::PING);

        FETCH_LOG_PROMISE();
        if (p->Wait(1000, false))
        {
          if (p->As<LaneIdentity::ping_type>() != LaneIdentity::PING_MAGIC)
          {
            connection_timeout = true;
          }
          break;
        }
      }
      else
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{200});
      }
    }

    if (connection_timeout)
    {
      FETCH_LOG_WARN(LOGGING_NAME,"Connection timed out - closing in StorageUnitClient::AddLaneConnection:1:");
      client->Close();
      client.reset();
      return crypto::InvalidIdentity();
    }

    // Exchaning info
    auto p1 = client->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_LANE_NUMBER);
    auto p2 = client->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_TOTAL_LANES);
    auto p3 = client->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_IDENTITY);

    FETCH_LOG_PROMISE();
    if ((!p1->Wait(1000, true)) || (!p2->Wait(1000, true)) || (!p3->Wait(1000, true)))
    {
      FETCH_LOG_WARN(LOGGING_NAME,"Client timeout when trying to get identity details.");
      client->Close();
      client.reset();
      return crypto::InvalidIdentity();
    }

    lane_type lane        = p1->As<lane_type>();
    lane_type total_lanes = p2->As<lane_type>();
    if (total_lanes > lanes_.size())
    {
      lanes_.resize(total_lanes);
      SetLaneLog2(uint32_t(lanes_.size()));
      assert(lanes_.size() == (1u << log2_lanes_));
    }

    crypto::Identity lane_identity;
    p3->As(lane_identity);
    // TODO(issue 24): Verify expected identity

    assert(lane < lanes_.size());
    FETCH_LOG_DEBUG(LOGGING_NAME,"Adding lane ", lane);

    lanes_[lane] = client;

    {
      auto details      = register_.GetDetails(client->handle());
      details->identity = lane_identity;
    }

    return lane_identity;
  }

  void TryConnect(p2p::EntryPoint const &ep)
  {
    if (ep.lane_id < lanes_.size())
    {
      lanes_[ep.lane_id]->Call(RPC_CONTROLLER, LaneControllerProtocol::TRY_CONNECT, ep);
    }
  }

  void AddTransaction(chain::VerifiedTransaction const &tx) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<chain::Transaction>;

    auto        res     = fetch::storage::ResourceID(tx.digest());
    std::size_t lane    = res.lane(log2_lanes_);
    auto        promise = lanes_[lane]->Call(RPC_TX_STORE, protocol::SET, res, tx);

    FETCH_LOG_PROMISE();
    promise->Wait();
  }

  bool GetTransaction(byte_array::ConstByteArray const &digest, chain::Transaction &tx) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<chain::Transaction>;

    auto        res     = fetch::storage::ResourceID(digest);
    std::size_t lane    = res.lane(log2_lanes_);
    auto        promise = lanes_[lane]->Call(RPC_TX_STORE, protocol::GET, res);
    tx                  = promise->As<chain::VerifiedTransaction>();

    return true;
  }

  Document GetOrCreate(ResourceAddress const &key) override
  {
    std::size_t lane = key.lane(log2_lanes_);

    auto promise = lanes_[lane]->Call(
        RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::GET_OR_CREATE,
        key.as_resource_id());

    return promise->As<storage::Document>();
  }

  Document Get(ResourceAddress const &key) override
  {
    std::size_t lane = key.lane(log2_lanes_);

    auto promise =
        lanes_[lane]->Call(RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::GET,
                           key.as_resource_id());

    return promise->As<storage::Document>();
  }

  bool Lock(ResourceAddress const &key) override
  {
    std::size_t lane = key.lane(log2_lanes_);

    auto promise = lanes_[lane]->Call(RPC_STATE,
                                      fetch::storage::RevertibleDocumentStoreProtocol::LOCK,
                                      key.as_resource_id());

    return promise->As<bool>();
  }

  bool Unlock(ResourceAddress const &key) override
  {
    std::size_t lane = key.lane(log2_lanes_);

    auto promise = lanes_[lane]->Call(RPC_STATE,
                                      fetch::storage::RevertibleDocumentStoreProtocol::UNLOCK,
                                      key.as_resource_id());

    return promise->As<bool>();
  }

  void Set(ResourceAddress const &key, StateValue const &value) override
  {
    std::size_t lane = key.lane(log2_lanes_);

    auto promise =
        lanes_[lane]->Call(RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::SET,
                           key.as_resource_id(), value);

    FETCH_LOG_PROMISE();
    promise->Wait(2000, true);
  }

  void Commit(bookmark_type const &bookmark) override
  {
    std::vector<service::Promise> promises;
    for (std::size_t i = 0; i < lanes_.size(); ++i)
    {
      auto promise = lanes_[i]->Call(
          RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::COMMIT, bookmark);
      promises.push_back(promise);
    }

    for (auto &p : promises)
    {
      FETCH_LOG_PROMISE();
      p->Wait();
    }
  }

  void Revert(bookmark_type const &bookmark) override
  {
    std::vector<service::Promise> promises;
    for (std::size_t i = 0; i < lanes_.size(); ++i)
    {
      auto promise = lanes_[i]->Call(
          RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::REVERT, bookmark);
      promises.push_back(promise);
    }

    for (auto &p : promises)
    {
      FETCH_LOG_PROMISE();
      p->Wait();
    }
  }

  byte_array::ConstByteArray Hash() override
  {
    // TODO(issue 33): Which lane?

    if (lanes_.empty())
    {
      TODO_FAIL("Lanes array empty when trying to get a hash from L0");
    }
    if (!lanes_[0])
    {
      TODO_FAIL("L0 null when trying to get a hash from L0");
    }

    return lanes_[0]
        ->Call(RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::HASH)
        ->As<byte_array::ByteArray>();
  }

  void SetID(byte_array::ByteArray const &id) { id_ = id; }

  byte_array::ByteArray const &id() { return id_; }

  std::size_t lanes() const { return lanes_.size(); }

  bool is_alive() const
  {
    bool alive = true;
    for (auto &lane : lanes_)
    {
      if (!lane->is_alive())
      {
        alive = false;
        break;
      }
    }
    return alive;
  }

private:
  network_manager_type network_manager_;
  uint32_t             log2_lanes_ = 0;

  void SetLaneLog2(lane_type const &count)
  {
    log2_lanes_ = uint32_t((sizeof(uint32_t) << 3) - uint32_t(__builtin_clz(uint32_t(count)) + 1));
  }

  client_register_type                    register_;
  byte_array::ByteArray                   id_;
  std::vector<shared_service_client_type> lanes_;
};

}  // namespace ledger
}  // namespace fetch
