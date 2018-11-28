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

#include <chrono>
#include <thread>
#include <utility>

namespace fetch {
namespace ledger {

class LaneConnectorWorker;

class StorageUnitClient : public StorageUnitInterface
{
public:
  struct ClientDetails
  {
    crypto::Identity      identity;
    std::atomic<uint32_t> lane{0};
  };

  using LaneIndex            = LaneIdentity::lane_type;
  using ServiceClient        = service::ServiceClient;
  using SharedServiceClient  = std::shared_ptr<ServiceClient>;
  using WeakServiceClient    = std::weak_ptr<ServiceClient>;
  using SharedServiceClients = std::map<LaneIndex, SharedServiceClient>;
  using ClientRegister       = fetch::network::ConnectionRegister<ClientDetails>;
  using Handle               = ClientRegister::connection_handle_type;
  using NetworkManager       = fetch::network::NetworkManager;
  using PromiseState         = fetch::service::PromiseState;
  using Promise              = service::Promise;
  using FutureTimepoint      = network::FutureTimepoint;
  using Mutex                = fetch::mutex::Mutex;
  using LockT                = std::lock_guard<Mutex>;
  using Peer                 = fetch::network::Peer;

  static constexpr char const *LOGGING_NAME = "StorageUnitClient";

  explicit StorageUnitClient(NetworkManager const &tm)
    : network_manager_(tm)
  {}

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
  template <typename T>
  size_t AddLaneConnectionsWaiting(
      const std::map<LaneIndex, Peer> &lanes,
      const std::chrono::milliseconds &timeout = std::chrono::milliseconds(1000))
  {
    AddLaneConnections<T>(lanes, timeout);
    for (;;)
    {
      {
        FETCH_LOCK(mutex_);
        if (bg_work_.CountPending() < 1)
        {
          break;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    bg_work_.DiscardFailures();
    bg_work_.DiscardTimeouts();

    FETCH_LOCK(mutex_);
    return lanes_.size();
  }

  bool ClientForLaneConnected(LaneIndex lane)
  {
    FETCH_LOCK(mutex_);
    auto iter = lanes_.find(lane);
    return (iter != lanes_.end());
  }

  SharedServiceClient GetClientForLane(LaneIndex lane)
  {
    LockT lock(mutex_);
    auto  iter = lanes_.find(lane);
    if (iter != lanes_.end())
    {
      return iter->second;
    }
    return SharedServiceClient();
  }

  void AddTransaction(chain::VerifiedTransaction const &tx) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<chain::Transaction>;

    auto      res     = fetch::storage::ResourceID(tx.digest());
    LaneIndex lane    = res.lane(log2_lanes_);
    auto      promise = lanes_.at(lane)->Call(RPC_TX_STORE, protocol::SET, res, tx);

    FETCH_LOG_PROMISE();
    promise->Wait();
  }

  void AddTransactions(TransactionList const &txs) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<chain::Transaction>;

    std::vector<protocol::ElementList> transaction_lists(1 << log2_lanes_);

    for (auto const &tx : txs)
    {
      // determine the lane for this given transaction
      auto      res  = fetch::storage::ResourceID(tx.digest());
      LaneIndex lane = res.lane(log2_lanes_);

      transaction_lists.at(lane).push_back({res, tx});
    }

    std::vector<Promise> promises;
    promises.reserve(1 << log2_lanes_);

    // dispatch all the set requests off
    {
      LaneIndex lane{0};
      for (auto const &list : transaction_lists)
      {
        if (!list.empty())
        {
          promises.emplace_back(lanes_.at(lane)->Call(RPC_TX_STORE, protocol::SET_BULK, list));
        }

        ++lane;
      }
    }

    // wait for all the requests to complete
    for (auto &promise : promises)
    {
      promise->Wait();
    }
  }

  bool GetTransaction(byte_array::ConstByteArray const &digest, chain::Transaction &tx) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<chain::Transaction>;

    auto      res     = fetch::storage::ResourceID(digest);
    LaneIndex lane    = res.lane(log2_lanes_);
    auto      promise = lanes_[lane]->Call(RPC_TX_STORE, protocol::GET, res);
    tx                = promise->As<chain::VerifiedTransaction>();

    return true;
  }

  Document GetOrCreate(ResourceAddress const &key) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    auto promise = lanes_[lane]->Call(
        RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::GET_OR_CREATE,
        key.as_resource_id());

    return promise->As<storage::Document>();
  }

  Document Get(ResourceAddress const &key) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    auto promise = lanes_[lane]->Call(
        RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::GET, key.as_resource_id());

    return promise->As<storage::Document>();
  }

  bool Lock(ResourceAddress const &key) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    auto promise = lanes_[lane]->Call(
        RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::LOCK, key.as_resource_id());

    return promise->As<bool>();
  }

  bool Unlock(ResourceAddress const &key) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    auto promise = lanes_[lane]->Call(
        RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::UNLOCK, key.as_resource_id());

    return promise->As<bool>();
  }

  void Set(ResourceAddress const &key, StateValue const &value) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    auto promise =
        lanes_[lane]->Call(RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::SET,
                           key.as_resource_id(), value);

    FETCH_LOG_PROMISE();
    promise->Wait();
  }

  void Commit(bookmark_type const &bookmark) override
  {
    std::vector<service::Promise> promises;
    for (auto const &lanedata : lanes_)
    {
      auto const &client  = lanedata.second;
      auto        promise = client->Call(
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
    for (auto const &lanedata : lanes_)
    {
      auto const &client  = lanedata.second;
      auto        promise = client->Call(
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

  std::size_t lanes() const
  {
    return lanes_.size();
  }

  bool IsAlive() const
  {
    bool alive = true;
    for (auto const &lanedata : lanes_)
    {
      auto const &client = lanedata.second;
      if (!client->is_alive())
      {
        alive = false;
        break;
      }
    }
    return alive;
  }

private:
private:
  friend class LaneConnectorWorker;  // this will do work for us, it's
                                     // easier if it has access to our
                                     // types.
  enum class State
  {
    INITIAL = 0,
    CONNECTING,
    QUERYING,
    PINGING,
    SNOOZING,
    DONE,
    TIMEDOUT,
    FAILED,
  };

  using Worker                    = LaneConnectorWorker;
  using WorkerPtr                 = std::shared_ptr<Worker>;
  using BackgroundedWork          = network::BackgroundedWork<LaneConnectorWorker>;
  using BackgroundedWorkThread    = network::HasWorkerThread<BackgroundedWork>;
  using BackgroundedWorkThreadPtr = std::shared_ptr<BackgroundedWorkThread>;

  void WorkCycle();

  std::shared_ptr<LaneConnectorWorker> MakeWorker(LaneIndex lane, SharedServiceClient client,
                                                  std::string               name,
                                                  std::chrono::milliseconds timeout);
  template <typename T>
  void AddLaneConnections(
      const std::map<LaneIndex, Peer> &lanes,
      const std::chrono::milliseconds &timeout = std::chrono::milliseconds(1000))
  {
    if (!workthread_)
    {
      workthread_ =
          std::make_shared<BackgroundedWorkThread>(&bg_work_, [this]() { this->WorkCycle(); });
    }

    for (auto const &lane : lanes)
    {
      auto                lanenum = lane.first;
      auto                target  = lane.second;
      SharedServiceClient client  = register_.template CreateServiceClient<T>(
          network_manager_, target.address(), target.port());
      std::string name   = target.ToString();
      auto        worker = MakeWorker(lanenum, client, name, std::chrono::milliseconds(timeout));
      bg_work_.Add(worker);
    }
  }

  void SetLaneLog2(LaneIndex count)
  {
    log2_lanes_ = uint32_t((sizeof(uint32_t) << 3) - uint32_t(__builtin_clz(uint32_t(count)) + 1));
  }

  NetworkManager            network_manager_;
  uint32_t                  log2_lanes_ = 0;
  ClientRegister            register_;
  Mutex                     mutex_{__LINE__, __FILE__};
  SharedServiceClients      lanes_;
  BackgroundedWork          bg_work_;
  BackgroundedWorkThreadPtr workthread_;
};

}  // namespace ledger
}  // namespace fetch
