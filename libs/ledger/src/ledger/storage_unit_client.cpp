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
  using Peer            = fetch::network::Peer;
  using Promise         = StorageUnitClient::Promise;
  using PromiseState    = StorageUnitClient::PromiseState;
  using Uri             = StorageUnitClient::Uri;
  using State           = StorageUnitClient::State;
  using Client          = StorageUnitClient::Client;

  LaneIndex lane;
  Promise   lane_prom;
  Promise   count_prom;
  Promise   id_prom;
  Address   target_address;

  MuddleLaneConnectorWorker(LaneIndex thelane, std::string thename, Uri thepeer, Muddle &themuddle,
                            std::chrono::milliseconds thetimeout = std::chrono::milliseconds(10000))
    : lane(thelane)
    , name_(std::move(thename))
    , peer_(std::move(thepeer))
    , timeduration_(std::move(thetimeout))
    , muddle_(themuddle)
  {
    client_ = std::make_shared<Client>(muddle_.AsEndpoint(), Muddle::Address(), SERVICE_LANE,
                                       CHANNEL_RPC);

    this->Allow(State::CONNECTING, State::INITIAL)
        .Allow(State::QUERYING, State::CONNECTING)
        .Allow(State::SUCCESS, State::QUERYING)

        .Allow(State::TIMEDOUT, State::INITIAL)
        .Allow(State::TIMEDOUT, State::CONNECTING)
        .Allow(State::TIMEDOUT, State::QUERYING)

        .Allow(State::FAILED, State::INITIAL)

        .Allow(State::FAILED, State::PINGING);
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
      timeout_.Set(timeduration_);
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
      muddle_.AddPeer(peer_);
      currentstate = State::CONNECTING;
      return true;
    }
    case State::CONNECTING:
    {
      bool connected = muddle_.GetOutgoingConnectionAddress(peer_, target_address);
      if (!connected)
      {
        return false;
      }
      currentstate = State::QUERYING;
      lane_prom  = client_->CallSpecificAddress(target_address, RPC_IDENTITY,
                                               LaneIdentityProtocol::GET_LANE_NUMBER);
      count_prom = client_->CallSpecificAddress(target_address, RPC_IDENTITY,
                                                LaneIdentityProtocol::GET_TOTAL_LANES);
      id_prom    = client_->CallSpecificAddress(target_address, RPC_IDENTITY,
      ping_        = client->Call(RPC_IDENTITY, LaneIdentityProtocol::PING);
    case State::PINGING:
      std::vector<PromiseState> states;
      auto result = ping_->GetState();
      states.push_back(count_prom->GetState());
      states.push_back(id_prom->GetState());
      for (unsigned int i = 0; i < 4; i++)
      case PromiseState::FAILED:
        else
          currentstate = State::TIMEDOUT;
      return true;

    }  // end CONNECTING
      for (unsigned int i = 0; i < 4; i++)
      {
        if (states[i] == PromiseState::WAITING)
        {
          return false;
        }
      }

      // Must be 3 successes.

      currentstate = State::SUCCESS;
      return true;
    }
    default:
      currentstate = State::FAILED;
      return true;
    }
  }

private:
  Promise                   ping_prom_;
  std::shared_ptr<Client>   client_;
  std::string               name_;
  Uri                       peer_;
  std::chrono::milliseconds timeduration_;
  PendingConnectionCounter  counter_;
  Muddle &                  muddle_;
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

    FETCH_LOG_DEBUG(LOGGING_NAME, " PEND=", bg_work_.CountPending());
    FETCH_LOG_DEBUG(LOGGING_NAME, " SUCC=", bg_work_.CountSuccesses());
    FETCH_LOG_DEBUG(LOGGING_NAME, " FAIL=", bg_work_.CountFailures());
    FETCH_LOG_DEBUG(LOGGING_NAME, " TOUT=", bg_work_.CountTimeouts());

    for (auto &successful_worker : bg_work_.GetSuccesses(1000))
    {
      if (successful_worker)
      {
        auto lanenum = successful_worker->lane;
        FETCH_LOG_VARIABLE(lanenum);
        FETCH_LOG_DEBUG(LOGGING_NAME, " PROCESSING lane ", lanenum);

        LaneIndex        lane;
        LaneIndex        total_lanes;
        crypto::Identity lane_identity;

        successful_worker->lane_prom->As(lane);
        successful_worker->count_prom->As(total_lanes);
        successful_worker->id_prom->As(lane_identity);

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

    FETCH_LOG_DEBUG(LOGGING_NAME, "Lanes Connected   :", lanes_.size());
    FETCH_LOG_DEBUG(LOGGING_NAME, "log2_lanes_       :", log2_lanes_);
    FETCH_LOG_DEBUG(LOGGING_NAME, "total_lanecount   :", total_lanecount);
  }

  bg_work_.DiscardFailures();
  bg_work_.DiscardTimeouts();
}

void StorageUnitClient::AddLaneConnections(Muddle &                         muddle,
                                                 const std::map<LaneIndex, Uri> & lanes,
                                                 const std::chrono::milliseconds &timeout)
{
  if (!workthread_)
  {
    workthread_ =
        std::make_shared<BackgroundedWorkThread>(&bg_work_, [this]() { this->WorkCycle(); });
  }

  for (auto const &lane : lanes)
  {
    auto        lanenum = lane.first;
    auto        peer    = lane.second;
    std::string name    = peer.ToString();

    auto worker = std::make_shared<MuddleLaneConnectorWorker>(lanenum, name, peer, muddle,
                                                              std::chrono::milliseconds(timeout));
    bg_work_.Add(worker);
  }
}

size_t StorageUnitClient::AddLaneConnectionsWaiting(Muddle &                         muddle,
                                                          const std::map<LaneIndex, Uri> & lanes,
                                                          const std::chrono::milliseconds &timeout)
{
  AddLaneConnections(muddle, lanes, timeout);
  PendingConnectionCounter::Wait(FutureTimepoint(timeout));

  FETCH_LOCK(mutex_);
  FETCH_LOG_INFO(LOGGING_NAME, "Successfully connected ", lane_to_identity_map_.size(),
                 " lane(s).");
  return lane_to_identity_map_.size();
}

}  // namespace ledger
}  // namespace fetch
