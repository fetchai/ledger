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

class StorageUnitClient : public StorageUnitInterface
{
private:
  class LaneConnectorWorker;

public:
  struct ClientDetails
  {
    crypto::Identity      identity;
    std::atomic<uint32_t> lane;
  };

  using LaneIndex               = LaneIdentity::lane_type;
  using ServiceClient           = service::ServiceClient;
  using SharedServiceClient     = std::shared_ptr<ServiceClient>;
  using WeakServiceClient       = std::weak_ptr<ServiceClient>;
  using SharedServiceClients    = std::map<LaneIndex, SharedServiceClient>;
  using ClientRegister          = fetch::network::ConnectionRegister<ClientDetails>;
  using Handle                  = ClientRegister::connection_handle_type;
  using NetworkManager          = fetch::network::NetworkManager;
  using PromiseState            = fetch::service::PromiseState;
  using Promise                 = service::Promise;
  using FutureTimepoint         = network::FutureTimepoint;
  using BackgroundedWork        = network::BackgroundedWork<LaneConnectorWorker>;
  using Worker                  = LaneConnectorWorker;
  using WorkerP                 = std::shared_ptr<Worker>;
  using BackgroundedWorkThread  = network::HasWorkerThread<BackgroundedWork>;
  using BackgroundedWorkThreadP = std::shared_ptr<BackgroundedWorkThread>;
  using Mutex                   = fetch::mutex::Mutex;
  using LockT                   = std::lock_guard<Mutex>;
  using Peer                    = fetch::network::Peer;

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

private:
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

  class LaneConnectorWorker : public network::AtomicStateMachine<State>
  {
  public:
    SharedServiceClient client;
    FutureTimepoint     next_attempt;
    size_t              attempts;
    Promise             ping;

    Promise               lane_prom;
    Promise               count_prom;
    Promise               id_prom;
    byte_array::ByteArray host;
    size_t                lane;
    uint16_t              port;
    std::string           name;
    FutureTimepoint       timeout;
    size_t                max_attempts;

    LaneConnectorWorker(
        size_t thelane, SharedServiceClient theclient, std::string const &thename,
        const std::chrono::milliseconds &thetimeout = std::chrono::milliseconds(1000))
    {
      lane = thelane;

      this->Allow(State::CONNECTING, State::INITIAL)
          .Allow(State::CONNECTING, State::SNOOZING)
          .Allow(State::PINGING, State::CONNECTING)
          .Allow(State::QUERYING, State::PINGING)
          .Allow(State::DONE, State::QUERYING)

          .Allow(State::INITIAL, State::CONNECTING)
          .Allow(State::INITIAL, State::PINGING)
          .Allow(State::INITIAL, State::QUERYING)

          .Allow(State::SNOOZING, State::CONNECTING)
          .Allow(State::SNOOZING, State::PINGING)
          .Allow(State::SNOOZING, State::QUERYING)

          .Allow(State::TIMEDOUT, State::CONNECTING)
          .Allow(State::TIMEDOUT, State::PINGING)
          .Allow(State::TIMEDOUT, State::QUERYING)

          .Allow(State::FAILED, State::CONNECTING);
      client       = theclient;
      attempts     = 0;
      name         = thename;
      timeout      = thetimeout;
      max_attempts = 10;
    }

    static constexpr char const *LOGGING_NAME = "StorageUnitClient::LaneConnectorWorker";

    virtual ~LaneConnectorWorker()
    {}

    PromiseState Work()
    {
      this->AtomicStateMachine::Work();
      auto r = this->AtomicStateMachine::Get();

      switch (r)
      {
      case State::TIMEDOUT:
        return PromiseState::TIMEDOUT;
      case State::DONE:
        return PromiseState::SUCCESS;
      case State::FAILED:
        return PromiseState::FAILED;
      default:
        return PromiseState::WAITING;
      }
    }

    std::string GetStateName(int state)
    {
      const char *states[] = {
          "INITIAL", "CONNECTING", "QUERYING", "PINGING", "SNOOZING", "DONE", "TIMEDOUT", "FAILED",
      };
      return std::string(states[state]);
    }

    virtual bool PossibleNewState(State &currentstate) override
    {
      if (timeout.IsDue())
      {
        currentstate = State::TIMEDOUT;
        return true;
      }
      switch (currentstate)
      {
      case State::INITIAL:
      {
        // we no longer use a fixed number of attempts.
        currentstate = State::CONNECTING;
        return true;
      }
      case State::SNOOZING:
      {
        if (next_attempt.IsDue())
        {
          currentstate = State::CONNECTING;
          return true;
        }
        else
        {
          return false;
        }
      }
      case State::CONNECTING:
      {
        if (!client->is_alive())
        {
          FETCH_LOG_DEBUG(LOGGING_NAME, " Lane ", lane, " (", name_, ") not yet alive.");
          attempts++;
          if (attempts > max_attempts)
          {
            currentstate = State::TIMEDOUT;
          }
          next_attempt.SetMilliseconds(300);
          currentstate = State::SNOOZING;
          return true;
        }
        ping         = client->Call(RPC_IDENTITY, LaneIdentityProtocol::PING);
        currentstate = State::PINGING;
        return true;
      }  // end CONNECTING
      case State::PINGING:
      {
        auto r = ping->GetState();

        switch (r)
        {
        case PromiseState::TIMEDOUT:
        case PromiseState::FAILED:
        {
          currentstate = State::INITIAL;
          return true;
        }
        case PromiseState::SUCCESS:
        {
          if (ping->As<LaneIdentity::ping_type>() != LaneIdentity::PING_MAGIC)
          {
            currentstate = State::INITIAL;
            return true;
          }
          else
          {
            lane_prom    = client->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_LANE_NUMBER);
            count_prom   = client->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_TOTAL_LANES);
            id_prom      = client->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_IDENTITY);
            currentstate = State::QUERYING;
            return true;
          }
        }
        case PromiseState::WAITING:
          return false;
        }
      }  // end PINGING
      case State::QUERYING:
      {
        auto lp_st = lane_prom->GetState();
        auto ct_st = count_prom->GetState();
        auto id_st = id_prom->GetState();

        if (lp_st == PromiseState::FAILED || id_st == PromiseState::FAILED ||
            ct_st == PromiseState::FAILED || lp_st == PromiseState::TIMEDOUT ||
            id_st == PromiseState::TIMEDOUT || ct_st == PromiseState::TIMEDOUT)
        {
          currentstate = State::INITIAL;
          return true;
        }

        if (lp_st == PromiseState::SUCCESS || id_st == PromiseState::SUCCESS ||
            ct_st == PromiseState::SUCCESS)
        {
          currentstate = State::DONE;
          return true;
        }
        return false;

      }  // end QUERYING
      case State::TIMEDOUT:
        return false;
      case State::DONE:
        return false;
      case State::FAILED:
        return false;
      }
      return false;
    }
  };

  void WorkCycle(void)
  {
    auto p = bg_work_.CountPending();

    bg_work_.WorkCycle();

    if (p != bg_work_.CountPending() || bg_work_.CountSuccesses() > 0)
    {

      LaneIndex total_lanecount = 0;

      FETCH_LOG_DEBUG(LOGGING_NAME, " PEND=", bg_work_.CountPending());
      FETCH_LOG_DEBUG(LOGGING_NAME, " SUCC=", bg_work_.CountSuccesses());
      FETCH_LOG_DEBUG(LOGGING_NAME, " FAIL=", bg_work_.CountFailures());
      FETCH_LOG_DEBUG(LOGGING_NAME, " TOUT=", bg_work_.CountTimeouts());

      for (auto &successful_worker : bg_work_.Get(PromiseState::SUCCESS, 1000))
      {
        if (successful_worker)
        {
          auto lanenum = successful_worker->lane;
          FETCH_LOG_VARIABLE(lanenum);
          FETCH_LOG_DEBUG(LOGGING_NAME, " PROCESSING lane ", lanenum);

          WeakServiceClient wp(successful_worker->client);
          LaneIndex         lane;
          LaneIndex         total_lanes;

          successful_worker->lane_prom->As(lane);
          successful_worker->count_prom->As(total_lanes);

          {
            FETCH_LOCK(mutex_);
            lanes_[lane] = std::move(successful_worker->client);

            if (!total_lanecount)
            {
              total_lanecount = total_lanes;
            }
            else
            {
              assert(total_lanes == total_lanecount);
            }
          }

          SetLaneLog2(uint32_t(lanes_.size()));

          crypto::Identity lane_identity;
          successful_worker->id_prom->As(lane_identity);
          // TODO(issue 24): Verify expected identity

          {
            auto details      = register_.GetDetails(lanes_[lane]->handle());
            details->identity = lane_identity;
          }
        }
      }
      FETCH_LOG_DEBUG(LOGGING_NAME, "Lanes Connected   :", lanes_.size());
      FETCH_LOG_DEBUG(LOGGING_NAME, "log2_lanes_       :", log2_lanes_);
      FETCH_LOG_DEBUG(LOGGING_NAME, "total_lanecount   :", total_lanecount);
    }
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
      auto        worker = std::make_shared<LaneConnectorWorker>(lanenum, client, name,
                                                          std::chrono::milliseconds(timeout));
      bg_work_.Add(worker);
    }
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
    auto      promise = lanes_[lane]->Call(RPC_TX_STORE, protocol::SET, res, tx);

    FETCH_LOG_PROMISE();
    promise->Wait();
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
    promise->Wait(2000, true);
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
  void SetLaneLog2(LaneIndex count)
  {
    log2_lanes_ = uint32_t((sizeof(uint32_t) << 3) - uint32_t(__builtin_clz(uint32_t(count)) + 1));
  }

  NetworkManager          network_manager_;
  uint32_t                log2_lanes_ = 0;
  ClientRegister          register_;
  Mutex                   mutex_{__LINE__, __FILE__};
  SharedServiceClients    lanes_;
  BackgroundedWork        bg_work_;
  BackgroundedWorkThreadP workthread_;
};

}  // namespace ledger
}  // namespace fetch
