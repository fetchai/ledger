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
#include "network/generics/atomic_state_machine.hpp"
#include "network/generics/backgrounded_work.hpp"
#include "network/generics/future_timepoint.hpp"
#include "network/generics/has_worker_thread.hpp"
#include "network/management/connection_register.hpp"
#include "network/service/service_client.hpp"

#include "ledger/chain/transaction.hpp"
#include "storage/document_store_protocol.hpp"
#include "storage/object_store_protocol.hpp"

#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"

#include <chrono>
#include <thread>
#include <utility>

namespace fetch {
namespace ledger {

class MuddleLaneConnectorWorker;

class StorageUnitClient : public StorageUnitInterface
{
public:
  struct ClientDetails
  {
    crypto::Identity      identity;
    std::atomic<uint32_t> lane{0};
  };

  using Client    = muddle::rpc::Client;
  using ClientPtr = std::shared_ptr<Client>;
  using MuddleEp  = muddle::MuddleEndpoint;
  using Muddle    = muddle::Muddle;
  using Address   = Muddle::Address;  // == a crypto::Identity.identifier_
  using Uri       = Muddle::Uri;
  using Peer      = fetch::network::Peer;

  using LaneIndex            = LaneIdentity::lane_type;
  using ServiceClient        = service::ServiceClient;
  using SharedServiceClient  = std::shared_ptr<ServiceClient>;
  using WeakServiceClient    = std::weak_ptr<ServiceClient>;
  using SharedServiceClients = std::map<LaneIndex, SharedServiceClient>;
  using LaneToIdentity       = std::map<LaneIndex, Address>;
  using ClientRegister       = fetch::network::ConnectionRegister<ClientDetails>;
  using Handle               = ClientRegister::connection_handle_type;
  using NetworkManager       = fetch::network::NetworkManager;
  using PromiseState         = fetch::service::PromiseState;
  using Promise              = service::Promise;
  using FutureTimepoint      = network::FutureTimepoint;
  using Mutex                = fetch::mutex::Mutex;
  using LockT                = std::lock_guard<Mutex>;

  static constexpr char const *LOGGING_NAME = "StorageUnitClient";

  explicit StorageUnitClient(NetworkManager const &tm, Muddle &muddle)
    : network_manager_(tm)
  {
    client_ =
        std::make_shared<Client>(muddle.AsEndpoint(), Muddle::Address(), SERVICE_LANE, CHANNEL_RPC);
  }

  StorageUnitClient(StorageUnitClient const &) = delete;
  StorageUnitClient(StorageUnitClient &&)      = delete;

  StorageUnitClient &operator=(StorageUnitClient const &) = delete;
  StorageUnitClient &operator=(StorageUnitClient &&) = delete;

  void SetNumberOfLanes(LaneIndex count)
  {
    SetLaneLog2(count);
    assert(count == (1u << log2_lanes_));
  }

public:
  size_t AddLaneConnectionsWaitingMuddle(
      Muddle &muddle, const std::map<LaneIndex, Uri> &lanes,
      const std::chrono::milliseconds &timeout = std::chrono::milliseconds(1000));

  void AddLaneConnectionsMuddle(
      Muddle &muddle, const std::map<LaneIndex, Uri> &lanes,
      const std::chrono::milliseconds &timeout = std::chrono::milliseconds(10000));

  bool GetAddressForLane(LaneIndex lane, Address &address) const
  {
    FETCH_LOCK(mutex_);

    auto iter = lane_to_identity_map_.find(lane);
    if (iter != lane_to_identity_map_.end())
    {
      address = iter->second;
      return true;
    }
    return false;
  }

  ClientPtr GetClient()
  {
    return client_;
  }

  void AddTransaction(chain::VerifiedTransaction const &tx) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<chain::Transaction>;

    auto      res  = fetch::storage::ResourceID(tx.digest());
    LaneIndex lane = res.lane(log2_lanes_);

    Address address;
    if (!GetAddressForLane(lane, address))
    {
      TODO_FAIL("Could not get address for lane ", lane);
    }
    auto promise = client_->CallSpecificAddress(address, RPC_TX_STORE, protocol::SET, res, tx);
    FETCH_LOG_PROMISE();
    promise->Wait();
  }

  bool GetTransaction(byte_array::ConstByteArray const &digest, chain::Transaction &tx) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<chain::Transaction>;

    auto      res  = fetch::storage::ResourceID(digest);
    LaneIndex lane = res.lane(log2_lanes_);
    Address   address;
    if (!GetAddressForLane(lane, address))
    {
      TODO_FAIL("Could not get address for lane ", lane);
    }

    auto promise = client_->CallSpecificAddress(address, RPC_TX_STORE, protocol::GET, res);
    tx           = promise->As<chain::VerifiedTransaction>();

    return true;
  }

  Document GetOrCreate(ResourceAddress const &key) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    Address address;
    if (!GetAddressForLane(lane, address))
    {
      TODO_FAIL("Could not get address for lane ", lane);
    }

    auto promise = client_->CallSpecificAddress(
        address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::GET_OR_CREATE,
        key.as_resource_id());

    return promise->As<storage::Document>();
  }

  Document Get(ResourceAddress const &key) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    Address address;
    if (!GetAddressForLane(lane, address))
    {
      TODO_FAIL("Could not get address for lane ", lane);
    }

    auto promise = client_->CallSpecificAddress(
        address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::GET,
        key.as_resource_id());

    return promise->As<storage::Document>();
  }

  bool Lock(ResourceAddress const &key) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    Address address;
    if (!GetAddressForLane(lane, address))
    {
      TODO_FAIL("Could not get address for lane ", lane);
    }

    auto promise = client_->CallSpecificAddress(
        address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::LOCK,
        key.as_resource_id());

    return promise->As<bool>();
  }

  bool Unlock(ResourceAddress const &key) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    Address address;
    if (!GetAddressForLane(lane, address))
    {
      TODO_FAIL("Could not get address for lane ", lane);
    }

    auto promise = client_->CallSpecificAddress(
        address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::UNLOCK,
        key.as_resource_id());

    return promise->As<bool>();
  }

  void Set(ResourceAddress const &key, StateValue const &value) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    Address address;
    if (!GetAddressForLane(lane, address))
    {
      TODO_FAIL("Could not get address for lane ", lane);
    }

    auto promise = client_->CallSpecificAddress(
        address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::SET,
        key.as_resource_id(), value);

    FETCH_LOG_PROMISE();
    promise->Wait();
  }

  void Commit(bookmark_type const &bookmark) override
  {
    std::vector<service::Promise> promises;
    for (auto const &lanedata : lane_to_identity_map_)
    {
      auto const &address = lanedata.second;
      auto        promise = client_->CallSpecificAddress(
          address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::COMMIT, bookmark);
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
    for (auto const &lanedata : lane_to_identity_map_)
    {
      auto const &address = lanedata.second;
      auto        promise = client_->CallSpecificAddress(
          address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::REVERT, bookmark);
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
    Address address;
    if (!GetAddressForLane(0, address))
    {
      TODO_FAIL("Lanes array empty when trying to get a hash from L0");
    }
    return client_
        ->CallSpecificAddress(address, RPC_STATE,
                              fetch::storage::RevertibleDocumentStoreProtocol::HASH)
        ->As<byte_array::ByteArray>();
  }

  std::size_t lanes() const
  {
    return lane_to_identity_map_.size();
  }

  bool IsAlive() const
  {
    return true;
  }

private:
private:
  friend class LaneConnectorWorkerInterface;
  friend class MuddleLaneConnectorWorker;
  friend class LaneConnectorWorker;  // these will do work for us, it's
                                     // easier if it has access to our
                                     // types.
  enum class State
  {
    INITIAL = 0,
    CONNECTING,
    QUERYING,
    PINGING,
    SNOOZING,
    SUCCESS,
    TIMEDOUT,
    FAILED,
  };

  using Worker                  = MuddleLaneConnectorWorker;
  using WorkerP                 = std::shared_ptr<Worker>;
  using BackgroundedWork        = network::BackgroundedWork<Worker>;
  using BackgroundedWorkThread  = network::HasWorkerThread<BackgroundedWork>;
  using BackgroundedWorkThreadP = std::shared_ptr<BackgroundedWorkThread>;

  void WorkCycle();

  void SetLaneLog2(LaneIndex count)
  {
    log2_lanes_ = uint32_t((sizeof(uint32_t) << 3) - uint32_t(__builtin_clz(uint32_t(count)) + 1));
  }

  NetworkManager          network_manager_;
  uint32_t                log2_lanes_ = 0;
  ClientPtr               client_;
  mutable Mutex           mutex_{__LINE__, __FILE__};
  LaneToIdentity          lane_to_identity_map_;
  BackgroundedWork        bg_work_;
  BackgroundedWorkThreadP workthread_;
};

}  // namespace ledger
}  // namespace fetch
