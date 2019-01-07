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

using PendingConnectionCounter =
    network::AtomicInFlightCounter<network::AtomicCounterName::LOCAL_SERVICE_CONNECTIONS>;

class MuddleLaneConnectorWorker : public network::AtomicStateMachine<StorageUnitClient::State>
{
public:
  using Address         = StorageUnitClient::Address;
  using FutureTimepoint = StorageUnitClient::FutureTimepoint;
  using LaneIndex       = StorageUnitClient::LaneIndex;
  using Muddle          = StorageUnitClient::Muddle;
  using MuddlePtr       = std::shared_ptr<Muddle>;
  using Peer            = fetch::network::Peer;
  using Promise         = StorageUnitClient::Promise;
  using PromiseState    = StorageUnitClient::PromiseState;
  using Uri             = StorageUnitClient::Uri;
  using State           = StorageUnitClient::State;
  using Client          = StorageUnitClient::Client;

  LaneIndex lane;
  Promise   lane_prom;
  Promise   count_prom;
  Address   target_address;

  MuddleLaneConnectorWorker(LaneIndex thelane, std::string thename, Uri thepeer,
                            MuddlePtr                 themuddle,
                            std::chrono::milliseconds thetimeout = std::chrono::milliseconds(10000))
    : lane(thelane)
    , name_(std::move(thename))
    , peer_(std::move(thepeer))
    , timeduration_(std::move(thetimeout))
    , muddle_(std::move(themuddle))
  {
    client_ = std::make_shared<Client>(muddle_->AsEndpoint(), Muddle::Address(), SERVICE_LANE,
                                       CHANNEL_RPC);

    this->Allow(State::CONNECTING, State::INITIAL)
        .Allow(State::QUERYING, State::CONNECTING)
        .Allow(State::SUCCESS, State::QUERYING)

        .Allow(State::TIMEDOUT, State::INITIAL)
        .Allow(State::TIMEDOUT, State::CONNECTING)
        .Allow(State::TIMEDOUT, State::QUERYING)

        .Allow(State::FAILED, State::CONNECTING)
        .Allow(State::FAILED, State::QUERYING)
        .Allow(State::FAILED, State::INITIAL);
  }
  static constexpr char const *LOGGING_NAME = "MuddleLaneConnectorWorker";

  virtual ~MuddleLaneConnectorWorker()
  {
    counter_.Completed();
  }

  PromiseState Work()
  {
    try
    {
      this->AtomicStateMachine::Work();
    }
    catch (...)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "MuddleLaneConnectorWorker::Work lane ", lane, " threw.");
      throw;
    }
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

    if (currentstate == State::INITIAL)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Timeout for lane ", lane, " set ", timeduration_.count());
      timeout_.Set(timeduration_);
    }

    if (timeout_.IsDue())
    {
      currentstate = State::TIMEDOUT;
      FETCH_LOG_INFO(LOGGING_NAME, "Timeout for lane ", lane);
      return true;
    }

    switch (currentstate)
    {
    case State::INITIAL:
    {
      FETCH_LOG_INFO(LOGGING_NAME, "INITIAL lane ", lane);
      muddle_->AddPeer(peer_);
      currentstate = State::CONNECTING;
      return true;
    }
    case State::CONNECTING:
    {
      bool connected = muddle_->UriToDirectAddress(peer_, target_address);
      if (!connected)
      {
        return false;
      }
      currentstate = State::QUERYING;
      lane_prom    = client_->CallSpecificAddress(target_address, RPC_IDENTITY,
                                               LaneIdentityProtocol::GET_LANE_NUMBER);
      count_prom   = client_->CallSpecificAddress(target_address, RPC_IDENTITY,
                                                LaneIdentityProtocol::GET_TOTAL_LANES);

      currentstate = State::QUERYING;
      return true;
    }
    case State::QUERYING:
    {
      auto p1 = count_prom->GetState();
      auto p2 = lane_prom->GetState();

      if ((p1 == PromiseState::FAILED) || (p2 == PromiseState::FAILED))
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Querying failed for lane ", lane);
        currentstate = State::FAILED;
        return true;
      }

      if ((p1 == PromiseState::WAITING) || (p2 == PromiseState::WAITING))
      {
        FETCH_LOG_INFO(LOGGING_NAME, "WAITING lane ", lane);
        return false;
      }

      currentstate = State::SUCCESS;
      return true;
    }
    default:
      FETCH_LOG_INFO(LOGGING_NAME, "Defaulted to fail for lane ", lane);
      currentstate = State::FAILED;
      return true;
    }
  }

private:
  std::shared_ptr<Client>   client_;
  std::string               name_;
  Uri                       peer_;
  std::chrono::milliseconds timeduration_;
  PendingConnectionCounter  counter_;
  MuddlePtr                 muddle_;
  FutureTimepoint           timeout_;
};

void StorageUnitClient::WorkCycle()
{
  if (!bg_work_.WorkCycle())
  {
    return;
  }

  if (bg_work_.CountSuccesses() > 0)
  {
    LaneIndex total_lanecount = 0;

    FETCH_LOG_INFO(LOGGING_NAME, " PEND=", bg_work_.CountPending());
    FETCH_LOG_INFO(LOGGING_NAME, " SUCC=", bg_work_.CountSuccesses());
    FETCH_LOG_INFO(LOGGING_NAME, " FAIL=", bg_work_.CountFailures());
    FETCH_LOG_INFO(LOGGING_NAME, " TOUT=", bg_work_.CountTimeouts());

    for (auto &successful_worker : bg_work_.GetSuccesses(1000))
    {
      if (successful_worker)
      {
        auto lanenum = successful_worker->lane;
        FETCH_LOG_VARIABLE(lanenum);
        FETCH_LOG_INFO(LOGGING_NAME, " PROCESSING lane ", lanenum);

        LaneIndex        lane;
        LaneIndex        total_lanes;
        crypto::Identity lane_identity;

        successful_worker->lane_prom->As(lane);
        successful_worker->count_prom->As(total_lanes);

        // TODO(issue 24): Verify expected identity

        {
          FETCH_LOCK(mutex_);
          lane_to_identity_map_[lane] = std::move(successful_worker->target_address);

          if (!total_lanecount)
          {
            total_lanecount = total_lanes;
            SetLaneLog2(uint32_t(total_lanecount));
          }
          else
          {
            assert(total_lanes == total_lanecount);
          }
        }
      }
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Lanes Connected   :", lane_to_identity_map_.size());
    FETCH_LOG_INFO(LOGGING_NAME, "log2_lanes_       :", log2_lanes_);
    FETCH_LOG_INFO(LOGGING_NAME, "total_lanecount   :", total_lanecount);
  }

  bg_work_.DiscardFailures();
  bg_work_.DiscardTimeouts();
}

void StorageUnitClient::AddLaneConnections(const std::map<LaneIndex, Uri> & lanes,
                                           const std::chrono::milliseconds &timeout)
{
  if (!workthread_)
  {
    workthread_ =
        std::make_shared<BackgroundedWorkThread>(&bg_work_, [this]() { this->WorkCycle(); });
  }

  // number of lanes is the number of the last lane asked for +1
  LaneIndex m = lanes.empty() ? 1 : lanes.rbegin()->first + 1;

  if (m > muddles_.size())
  {
    SetNumberOfLanes(m);
  }

  for (auto const &lane : lanes)
  {
    auto        lanenum = lane.first;
    auto        peer    = lane.second;
    std::string name    = peer.ToString();

    auto worker = std::make_shared<MuddleLaneConnectorWorker>(
        lanenum, name, peer, GetMuddleForLane(lanenum), std::chrono::milliseconds(timeout));
    bg_work_.Add(worker);
  }
}

size_t StorageUnitClient::AddLaneConnectionsWaiting(const std::map<LaneIndex, Uri> & lanes,
                                                    const std::chrono::milliseconds &timeout)
{
  AddLaneConnections(lanes, timeout);
  PendingConnectionCounter::Wait(FutureTimepoint(timeout));

  std::size_t nLanes;
  {
    FETCH_LOCK(mutex_);
    nLanes = lane_to_identity_map_.size();
  }
  FETCH_LOG_INFO(LOGGING_NAME, "Successfully connected ", nLanes, " lane(s).");
  return nLanes;
}

}  // namespace ledger
}  // namespace fetch
