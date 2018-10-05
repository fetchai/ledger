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

std::shared_ptr<LaneConnectorWorker> StorageUnitClient::MakeWorker(
    LaneIndex lane, SharedServiceClient client, std::string name, std::chrono::milliseconds timeout)
{
  return std::make_shared<LaneConnectorWorker>(lane, client, name, timeout);
}

class LaneConnectorWorker : public network::AtomicStateMachine<StorageUnitClient::State>
{
public:
  using State               = StorageUnitClient::State;
  using SharedServiceClient = StorageUnitClient::SharedServiceClient;
  using Promise             = StorageUnitClient::Promise;
  using PromiseState        = StorageUnitClient::PromiseState;
  using LaneIndex           = StorageUnitClient::LaneIndex;
  using FutureTimepoint     = StorageUnitClient::FutureTimepoint;

  LaneIndex           lane;
  Promise             lane_prom;
  SharedServiceClient client;
  Promise             count_prom;
  Promise             id_prom;

  LaneConnectorWorker(LaneIndex thelane, SharedServiceClient theclient, std::string thename,
                      std::chrono::milliseconds thetimeout = std::chrono::milliseconds(1000))
    : lane(thelane)
    , client(std::move(theclient))
    , attempts_(0)
    , name_(std::move(thename))
    , timeout_(std::move(thetimeout))
    , max_attempts_(10)
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

void StorageUnitClient::WorkCycle(void)
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

}  // namespace ledger
}  // namespace fetch
