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
#include "network/service/service_client.hpp"
#include "network/generics/atomic_state_machine.hpp"
#include "network/generics/future_timepoint.hpp"
#include "network/generics/backgrounded_work.hpp"

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

  using ServiceClient        = service::ServiceClient;
  using SharedServiceClient  = std::shared_ptr<ServiceClient>;
  using WeakServiceClient    = std::weak_ptr<ServiceClient>;
  using SharedServiceClients = std::vector<SharedServiceClient>;
  using ClientRegister       = fetch::network::ConnectionRegister<ClientDetails>;
  using Handle               = ClientRegister::connection_handle_type;
  using NetworkManager       = fetch::network::NetworkManager;
  using LaneIndex            = LaneIdentity::lane_type;
  using PromiseState         = fetch::service::PromiseState;
  using AtomicStateMachine   = network::AtomicStateMachine;
  using Promise              = service::Promise;
  using FutureTimepoint      = network::FutureTimepoint;

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
    lanes_.resize(count);
    SetLaneLog2(count);
    assert(count == (1u << log2_lanes_));
  }


  class LaneConnectorWorker : public AtomicStateMachine
  {
  public:
    enum
      {
        INITIALISING = 1,
        CONNECTING,
        QUERYING,
        PINGING,
        SNOOZING,
        DONE,
        TIMEDOUT,
        FAILED,
      };

    SharedServiceClient client_;
    FutureTimepoint next_attempt_;
    size_t attempts_;
    Promise ping_;

    Promise lane_prom_;
    Promise count_prom_;
    Promise id_prom_;
    byte_array::ByteArray host_;
    size_t lane_;
    uint16_t port_;
    std::string name_;

    LaneConnectorWorker(size_t lane, SharedServiceClient client, const std::string &name)
    {
      lane_ = lane;

      this->Allow(LaneConnectorWorker::INITIALISING, 0)
        .Allow(CONNECTING, INITIALISING)
        .Allow(CONNECTING, SNOOZING)
        .Allow(PINGING, CONNECTING)
        .Allow(QUERYING, PINGING)
        .Allow(DONE, QUERYING)

        .Allow(INITIALISING, CONNECTING)
        .Allow(INITIALISING, PINGING)
        .Allow(INITIALISING, QUERYING)

        .Allow(SNOOZING, CONNECTING)
        .Allow(SNOOZING, PINGING)
        .Allow(SNOOZING, QUERYING)

        .Allow(TIMEDOUT, CONNECTING)
        .Allow(TIMEDOUT, PINGING)
        .Allow(TIMEDOUT, QUERYING)

        .Allow(FAILED, CONNECTING)
        ;
      client_ = client;
      attempts_ = 0;
      name_ = name;
    }
    static constexpr char const *LOGGING_NAME = "StorageUnitClient::LaneConnectorWorker";

    virtual ~LaneConnectorWorker()
    {
      if (client_)
      {
        client_->Close();
      }
    }

    PromiseState Work()
    {
      this->AtomicStateMachine::Work();
      auto r = this->AtomicStateMachine::Get();

      switch(r)
      {
      case TIMEDOUT:
        return PromiseState::TIMEDOUT;
      case DONE:
        return PromiseState::SUCCESS;
      case FAILED:
        return PromiseState::FAILED;
      default:
        return PromiseState::WAITING;
      }
    }

    std::string name(int state)
    {
      const char *states[] = {
        "(START)",
        "INITIALISING",
        "CONNECTING",
        "QUERYING",
        "PINGING",
        "SNOOZING",
        "DONE",
        "TIMEDOUT",
        "FAILED",
      };
      return std::string(states[state]);
    }

    virtual int PossibleNewState(int currentstate)
    {
      auto x = PossibleNewStateImpl(currentstate);
      if (x && x != currentstate)
      {
        FETCH_LOG_WARN(LOGGING_NAME," Statechange ", lane_, " from ", name(currentstate), " -> ", name(x));
      }
      return x;
    }

    virtual int PossibleNewStateImpl(int currentstate)
    {
      switch(currentstate)
      {
      case 0:
        {
          return INITIALISING;
        }
      case INITIALISING:
        {
          attempts_++;
          if (attempts_>10)
          {
            return TIMEDOUT;
          }
          return CONNECTING;
        }
      case SNOOZING:
        {
          if (next_attempt_.IsDue())
          {
            return CONNECTING;
          }
          else
          {
            return 0;
          }
        }
      case CONNECTING:
        {
          if (!client_->is_alive())
          {
            FETCH_LOG_WARN(LOGGING_NAME," Lane ", lane_, " (", name_, ") not yet alive.");
            attempts_++;
            if (attempts_>10)
            {
              return TIMEDOUT;
            }
            next_attempt_.SetMilliseconds(1000);
            return SNOOZING;
          }
          ping_ = client_->Call(RPC_IDENTITY, LaneIdentityProtocol::PING);
          return PINGING;
        } // end CONNECTING
      case PINGING:
        {
          auto r = ping_->GetState();

          switch(r)
          {
          case PromiseState::TIMEDOUT:
          case PromiseState::FAILED:
            {
              return INITIALISING;
            }
          case PromiseState::SUCCESS:
            {
              if (ping_->As<LaneIdentity::ping_type>() != LaneIdentity::PING_MAGIC)
              {
                return INITIALISING;
              }
              else
              {
                lane_prom_  = client_->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_LANE_NUMBER);
                count_prom_ = client_->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_TOTAL_LANES);
                id_prom_    = client_->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_IDENTITY);
                next_attempt_.SetMilliseconds(2000);
                return QUERYING;
              }
            }
          case PromiseState::WAITING:
            return 0;
          }
        } // end PINGING
      case QUERYING:
        {
          auto lp_st = lane_prom_->GetState();
          auto ct_st = count_prom_->GetState();
          auto id_st = id_prom_->GetState();

          if (lp_st == PromiseState::FAILED
              || id_st == PromiseState::FAILED
              || ct_st == PromiseState::FAILED
              || lp_st == PromiseState::TIMEDOUT
              || id_st == PromiseState::TIMEDOUT
              || ct_st == PromiseState::TIMEDOUT
              )
          {
            return INITIALISING;
          }

          if (lp_st == PromiseState::SUCCESS
              || id_st == PromiseState::SUCCESS
              || ct_st == PromiseState::SUCCESS)
          {
            return DONE;
          }

          if (next_attempt_.IsDue())
          {
            return TIMEDOUT;
          }
          return 0;

        } // end QUERYING
      case TIMEDOUT:
        return 0;
      case DONE:
        return 0;
      case FAILED:
        return 0;
      }
      return 0;
    }
  };


  template <typename T>
  std::vector<WeakServiceClient> AddLaneConnections(const std::vector<std::pair<byte_array::ByteArray, uint16_t>> &lanes)
  {
    using BackgroundedWork = network::BackgroundedWork<LaneConnectorWorker>;
    using Worker           = LaneConnectorWorker;
    using WorkerP          = std::shared_ptr<Worker>;

    BackgroundedWork workers;
    std::map<size_t, WorkerP> by_number;

    for(size_t i=0;i<lanes.size();i++)
    {
      SharedServiceClient client = register_.template CreateServiceClient<T>(network_manager_, lanes[i].first, lanes[i].second);
      std::string name;
      name += std::string(lanes[i].first);
      name += ":";
      name += std::to_string(lanes[i].second);
      by_number[i] = std::make_shared<LaneConnectorWorker>(i,client, name);
      workers.Add(by_number[i]);
    }


    while(workers.CountPending() > 0)
    {
      FETCH_LOG_WARN(LOGGING_NAME," TICK!");
      workers.WorkCycle();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    FETCH_LOG_WARN(LOGGING_NAME," PEND=", workers.CountPending());
    FETCH_LOG_WARN(LOGGING_NAME," SUCC=", workers.CountSuccesses());
    FETCH_LOG_WARN(LOGGING_NAME," FAIL=", workers.CountFailures());
    FETCH_LOG_WARN(LOGGING_NAME," TOUT=", workers.CountTimeouts());

    if (workers.CountSuccesses() != lanes.size())
    {
      TODO_FAIL("Could not connect all lanes ", workers.CountSuccesses(), "!=", lanes.size());
    }

    std::vector<WeakServiceClient> results;
    for(size_t i=0;i<lanes.size();i++)
    {
      FETCH_LOG_WARN(LOGGING_NAME," PROCESSING ", i);
      WorkerP connector = by_number[i];
      WeakServiceClient wp(connector -> client_);
      results.push_back(wp);

      LaneIndex lane;
      LaneIndex total_lanes;

      FETCH_LOG_WARN(LOGGING_NAME," GETTING STUFFS FROM ", i);
      connector->lane_prom_->As(lane);
      connector->count_prom_->As(total_lanes);

      if (total_lanes > lanes_.size())
      {
        FETCH_LOG_WARN(LOGGING_NAME," OMG RESIZE ", i);
        lanes_.resize(total_lanes);
        SetLaneLog2(uint32_t(lanes_.size()));
        assert(lanes_.size() == (1u << log2_lanes_));
      }

      FETCH_LOG_WARN(LOGGING_NAME,"WoooOOOOoooo Crypt for ", i);
      crypto::Identity lane_identity;
      connector->id_prom_->As(lane_identity);
      // TODO(issue 24): Verify expected identity

      assert(lane < lanes_.size());
      lanes_[lane] = connector->client_;
      FETCH_LOG_WARN(LOGGING_NAME,"Lane stored for ", i);

      {
        auto details      = register_.GetDetails(connector->client_->handle());
        details->identity = lane_identity;
      }
    }
    FETCH_LOG_WARN(LOGGING_NAME,"DONE!", lanes_.size());
    return results;
  }

  template <typename T>
  WeakServiceClient AddLaneConnection(byte_array::ByteArray const &host, uint16_t const &port)
  {
    SharedServiceClient client =
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
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Connection timed out - closing in StorageUnitClient::AddLaneConnection:1:");
      client->Close();
      client.reset();
      return {};
    }

    // Exchanging info
    auto p1 = client->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_LANE_NUMBER);
    auto p2 = client->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_TOTAL_LANES);
    auto p3 = client->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_IDENTITY);

    try {

      FETCH_LOG_PROMISE();
      if ((!p1->Wait(1000, true)) || (!p2->Wait(1000, true)) || (!p3->Wait(1000, true)))
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Client timeout when trying to get identity details.");
        client->Close();
        client.reset();
        return {};
      }

    }
    catch (...) {
      FETCH_LOG_WARN(LOGGING_NAME, "OMG FAILED TRYING StorageUnitClient get details from ", host, ":", port);
        throw;
      }

    LaneIndex lane        = p1->As<LaneIndex>();
    LaneIndex total_lanes = p2->As<LaneIndex>();
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
    lanes_[lane] = client;

    {
      auto details      = register_.GetDetails(client->handle());
      details->identity = lane_identity;
    }

    return client;
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

    auto promise = lanes_[lane]->Call(
        RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::GET, key.as_resource_id());

    return promise->As<storage::Document>();
  }

  bool Lock(ResourceAddress const &key) override
  {
    std::size_t lane = key.lane(log2_lanes_);

    auto promise = lanes_[lane]->Call(
        RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::LOCK, key.as_resource_id());

    return promise->As<bool>();
  }

  bool Unlock(ResourceAddress const &key) override
  {
    std::size_t lane = key.lane(log2_lanes_);

    auto promise = lanes_[lane]->Call(
        RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::UNLOCK, key.as_resource_id());

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

  std::size_t lanes() const
  {
    return lanes_.size();
  }

  bool IsAlive() const
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
  void SetLaneLog2(LaneIndex count)
  {
    log2_lanes_ = uint32_t((sizeof(uint32_t) << 3) - uint32_t(__builtin_clz(uint32_t(count)) + 1));
  }

  NetworkManager       network_manager_;
  uint32_t             log2_lanes_ = 0;
  ClientRegister       register_;
  SharedServiceClients lanes_;
};

}  // namespace ledger
}  // namespace fetch
