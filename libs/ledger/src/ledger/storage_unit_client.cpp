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

#include "ledger/storage_unit/storage_unit_client.hpp"

namespace fetch {
namespace ledger {

  class MuddleLaneConnectorWorker : public network::AtomicStateMachine<StorageUnitClient::State>
{
public:
  using Address        = StorageUnitClient::Address;
  using FutureTimepoint     = StorageUnitClient::FutureTimepoint;
  using LaneIndex           = StorageUnitClient::LaneIndex;
  using Muddle        = StorageUnitClient::Muddle;
  using Peer                 = fetch::network::Peer;
  using Promise             = StorageUnitClient::Promise;
  using PromiseState        = StorageUnitClient::PromiseState;
  using Uri        = StorageUnitClient::Uri;
  using State               = StorageUnitClient::State;
  using Client               = StorageUnitClient::Client;
  

  Promise             lane_prom;
  Promise             count_prom;
  Promise             id_prom;
  LaneIndex           lane;
  Promise             ping_prom;
  std::shared_ptr<Client> client;

  std::string         name;
  Uri peer;
  FutureTimepoint     timeout;
  Muddle &muddle;
  bool added;
  Address target;

  MuddleLaneConnectorWorker(LaneIndex thelane,  std::string thename, Uri thepeer,
                            Muddle &themuddle,
                            std::chrono::milliseconds thetimeout = std::chrono::milliseconds(1000)
                            )
   : lane(thelane)
   , name(std::move(thename))
   , peer(std::move(thepeer))
   , timeout(std::move(thetimeout))
   , muddle(themuddle)
  {
    client = std::make_shared<Client>(muddle.AsEndpoint(), Muddle::Address(), SERVICE_P2P, CHANNEL_RPC);

    this->Allow(State::CONNECTING, State::INITIAL)
      .Allow(State::QUERYING, State::CONNECTING)
      .Allow(State::SUCCESS, State::QUERYING)

      .Allow(State::TIMEDOUT, State::INITIAL)
      .Allow(State::TIMEDOUT, State::CONNECTING)
      .Allow(State::TIMEDOUT, State::QUERYING)

      .Allow(State::FAILED, State::INITIAL)
      .Allow(State::FAILED, State::CONNECTING)
      .Allow(State::FAILED, State::QUERYING)
      ;
  }

  PromiseState Work()
  {
    this->AtomicStateMachine::Work();
    auto r = this->AtomicStateMachine::Get();

    switch (r)
    {
    case State::TIMEDOUT:
      return PromiseState::TIMEDOUT;
    case State::SUCCESS:
      return PromiseState::SUCCESS;
    case State::FAILED:
      return PromiseState::FAILED;
    default:
      return PromiseState::WAITING;
    }
  }

  virtual bool PossibleNewState(State &currentstate) override
  {
    if (currentstate == State::TIMEDOUT || currentstate == State::SUCCESS ||
        currentstate == State::FAILED)
    {
      return false;
    }

    if (timeout.IsDue())
    {
      currentstate = State::TIMEDOUT;
      return true;
    }

    switch (currentstate)
    {
    case State::INITIAL:
      {
        muddle.AddPeer(peer);
        currentstate = State::CONNECTING;
        return true;
      }
    case State::CONNECTING:
    {
      bool connected = muddle.GetOutgoingConnectionAddress(peer, target);
      if (!connected)
      {
        return false;
      }
      currentstate = State::QUERYING;
      ping_prom = client->CallSpecificAddress(target, RPC_IDENTITY, LaneIdentityProtocol::PING);
      lane_prom = client->CallSpecificAddress(target, RPC_IDENTITY, LaneIdentityProtocol::GET_LANE_NUMBER);
      count_prom = client->CallSpecificAddress(target, RPC_IDENTITY, LaneIdentityProtocol::GET_TOTAL_LANES);
      id_prom = client->CallSpecificAddress(target, RPC_IDENTITY, LaneIdentityProtocol::GET_IDENTITY);
      return true;
    }
    case State::QUERYING:
    {
      std::vector<PromiseState> states;
      states.push_back(ping_prom->GetState());
      states.push_back(lane_prom->GetState());
      states.push_back(count_prom->GetState());
      states.push_back(id_prom->GetState());

      for(unsigned int i =0;i<4;i++)
      {
        if (states[i] ==  PromiseState::FAILED)
        {
          currentstate = State::FAILED;
          return true;
        }
        if (states[i] ==  PromiseState::TIMEDOUT)
        {
          currentstate = State::TIMEDOUT;
          return true;
        }
      }
      for(unsigned int i =0;i<4;i++)
      {
        if (states[i] ==  PromiseState::WAITING)
        {
          return false;
        }
      }

      // Must be 4 successes.

      if (ping_prom->As<LaneIdentity::ping_type>() != LaneIdentity::PING_MAGIC)
      {
        currentstate = State::FAILED;
        return true;
      }
       currentstate = State::SUCCESS;
       return true;
    }
    default:
      currentstate = State::FAILED;
      return true;
    }
  }
  };

  /*
class LaneConnectorWorker
  : public network::AtomicStateMachine<StorageUnitClient::State>
  , public LaneConnectorWorkerInterface
{
public:
  using State               = StorageUnitClient::State;
  using SharedServiceClient = StorageUnitClient::SharedServiceClient;
  using Promise             = StorageUnitClient::Promise;
  using PromiseState        = StorageUnitClient::PromiseState;
  using LaneIndex           = StorageUnitClient::LaneIndex;
  using FutureTimepoint     = StorageUnitClient::FutureTimepoint;

  SharedServiceClient client;

  LaneConnectorWorker(LaneIndex thelane, Peer thepeer, SharedServiceClient theclient, std::string thename,
                      std::chrono::milliseconds thetimeout = std::chrono::milliseconds(1000))
    : lane(thelane)
    , client(std::move(theclient))
    , attempts_(0)
    , name_(std::move(thename))
    , timeout_(std::move(thetimeout))
    , max_attempts_(10)
   , peer(std::move(thepeer))
  {
    // Ideal path first.
    this->Allow(State::CONNECTING, State::INITIAL)
        .Allow(State::PINGING, State::CONNECTING)
        .Allow(State::QUERYING, State::PINGING)
        .Allow(State::DONE, State::QUERYING)

        // Waiting for liveness.
        .Allow(State::SNOOZING, State::CONNECTING)
        .Allow(State::CONNECTING, State::SNOOZING)

        // Retrying from scratch.
        .Allow(State::INITIAL, State::PINGING)
        .Allow(State::INITIAL, State::QUERYING)

        // We can timeout from any of the non-finish states.
        .Allow(State::TIMEDOUT, State::INITIAL)
        .Allow(State::TIMEDOUT, State::CONNECTING)
        .Allow(State::TIMEDOUT, State::PINGING)
        .Allow(State::TIMEDOUT, State::QUERYING)
        .Allow(State::TIMEDOUT, State::SNOOZING)

        // Wrong magic causes us to bail with a failure.
        .Allow(State::FAILED, State::PINGING);
  }

  static constexpr char const *LOGGING_NAME = "StorageUnitClient::LaneConnectorWorker";

  virtual ~LaneConnectorWorker() = default;

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

  char const *GetStateName(State state)
  {
    switch (state)
    {
    case State::INITIAL:
      return "INITIAL";
    case State::CONNECTING:
      return "CONNECTING";
    case State::QUERYING:
      return "QUERYING";
    case State::PINGING:
      return "PINGING";
    case State::SNOOZING:
      return "SNOOZING";
    case State::DONE:
      return "DONE";
    case State::TIMEDOUT:
      return "TIMEDOUT";
    case State::FAILED:
      return "FAILED";
    default:
      return "?";
    }
  }

  virtual bool PossibleNewState(State &currentstate) override
  {

    if (currentstate == State::TIMEDOUT || currentstate == State::DONE ||
        currentstate == State::FAILED)
    {
      return false;
    }

    if (timeout_.IsDue())
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
      if (next_attempt_.IsDue())
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
        attempts_++;
        if (attempts_ > max_attempts_)
        {
          currentstate = State::TIMEDOUT;
        }
        next_attempt_.SetMilliseconds(300);
        currentstate = State::SNOOZING;
        return true;
      }
      ping_        = client->Call(RPC_IDENTITY, LaneIdentityProtocol::PING);
      currentstate = State::PINGING;
      return true;
    }  // end CONNECTING
    case State::PINGING:
    {
      auto result = ping_->GetState();

      switch (result)
      {
      case PromiseState::TIMEDOUT:
      case PromiseState::FAILED:
      {
        currentstate = State::INITIAL;
        return true;
      }
      case PromiseState::SUCCESS:
      {
        if (ping_->As<LaneIdentity::ping_type>() != LaneIdentity::PING_MAGIC)
        {
          currentstate = State::FAILED;
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

      return false;
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
    default:
      return false;
    }
  }

private:
  FutureTimepoint next_attempt_;
  size_t          attempts_;
  Promise         ping_;

  byte_array::ByteArray host_;
  std::string           name_;
  FutureTimepoint       timeout_;
  size_t                max_attempts_;
};

  */

void StorageUnitClient::WorkCycle()
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

        LaneIndex         lane;
        LaneIndex         total_lanes;

        successful_worker->lane_prom->As(lane);
        successful_worker->count_prom->As(total_lanes);

        {
          FETCH_LOCK(mutex_);
          //lanes_[lane] = std::move(successful_worker->client);
          lane_addresses_[lane] = std::move(successful_worker->target);

          if (!total_lanecount)
          {
            total_lanecount = total_lanes;
          }
          else
          {
            assert(total_lanes == total_lanecount);
          }
        }

        SetLaneLog2(uint32_t(lane_addresses_.size()));

        crypto::Identity lane_identity;
        successful_worker->id_prom->As(lane_identity);
        // TODO(issue 24): Verify expected identity

    //    {
    //      auto details      = register_.GetDetails(lanes_[lane]->handle());
    //      details->identity = lane_identity;
    //    }
      }
    }
    FETCH_LOG_DEBUG(LOGGING_NAME, "Lanes Connected   :", lanes_.size());
    FETCH_LOG_DEBUG(LOGGING_NAME, "log2_lanes_       :", log2_lanes_);
    FETCH_LOG_DEBUG(LOGGING_NAME, "total_lanecount   :", total_lanecount);
  }
}

  void StorageUnitClient::AddLaneConnectionsMuddle(
                                Muddle &muddle,
                                const std::map<LaneIndex, Uri> &lanes,
                                const std::chrono::milliseconds &timeout
                                )
  {
    if (!workthread_)
    {
      workthread_ =
          std::make_shared<BackgroundedWorkThread>(&bg_work_, [this]() { this->WorkCycle(); });
    }

    for (auto const &lane : lanes)
    {
      auto                lanenum = lane.first;
      auto                peer  = lane.second;
      std::string name   = peer.ToString();

      auto worker = std::make_shared<MuddleLaneConnectorWorker>(lanenum, name, peer, muddle, std::chrono::milliseconds(timeout));
      bg_work_.Add(worker);
    }
  }

}  // namespace ledger
}  // namespace fetch
