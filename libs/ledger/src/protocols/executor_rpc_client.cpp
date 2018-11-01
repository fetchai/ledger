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

#include "ledger/protocols/executor_rpc_client.hpp"

namespace fetch {
namespace ledger {

using PendingConnectionCounter =
    network::AtomicInFlightCounter<network::AtomicCounterName::LOCAL_SERVICE_CONNECTIONS>;

class ExecutorConnectorWorker : public network::AtomicStateMachine<ExecutorRpcClient::State>
{
public:
  using Address         = ExecutorRpcClient::Address;
  using FutureTimepoint = ExecutorRpcClient::FutureTimepoint;
  using Muddle          = ExecutorRpcClient::Muddle;
  using PromiseState    = ExecutorRpcClient::PromiseState;
  using Uri             = ExecutorRpcClient::Uri;
  using State           = ExecutorRpcClient::State;
  using Client          = ExecutorRpcClient::Client;

  Uri                       peer;
  std::chrono::milliseconds timeduration;
  FutureTimepoint           timeout;
  PendingConnectionCounter  counter_;
  std::shared_ptr<Client>   client;
  Muddle &                  muddle;
  Address                   target_address;

  ExecutorConnectorWorker(Uri thepeer, Muddle &themuddle,
                          std::chrono::milliseconds thetimeout = std::chrono::milliseconds(1000))
    : peer(std::move(thepeer))
    , timeout(std::move(thetimeout))
    , muddle(themuddle)
  {
    client = std::make_shared<Client>(muddle.AsEndpoint(), Muddle::Address(), SERVICE_EXECUTOR,
                                      CHANNEL_RPC);
    this->Allow(State::CONNECTING, State::INITIAL)
        .Allow(State::SUCCESS, State::CONNECTING)
        .Allow(State::TIMEDOUT, State::CONNECTING)
        .Allow(State::FAILED, State::CONNECTING);
  }

  virtual ~ExecutorConnectorWorker()
  {
    counter_.Completed();
  }

  static constexpr char const *LOGGING_NAME = "ExecutorConnectorWorker";

  PromiseState Work()
  {
    try
    {
      this->AtomicStateMachine::Work();
    }
    catch (...)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Work threw.");
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
      FETCH_LOG_WARN(LOGGING_NAME, "INITIAL -> set timeout ", timeduration.count(), " milliseconds.");
      timeout.Set(timeduration);
    }

    if (timeout.IsDue())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "INITIAL -> TIMEDOUT");
      currentstate = State::TIMEDOUT;
      return true;
    }

    switch (currentstate)
    {
    case State::INITIAL:
    {
      muddle.AddPeer(peer);
      currentstate = State::CONNECTING;
      FETCH_LOG_WARN(LOGGING_NAME, "INITIAL -> CONNECTING");
      return true;
    }
    case State::CONNECTING:
    {
      bool connected = muddle.GetOutgoingConnectionAddress(peer, target_address);
      if (!connected)
      {
        return false;
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

void ExecutorRpcClient::Connect(Muddle &muddle, Uri uri, std::chrono::milliseconds timeout)
{
  if (!workthread_)
  {
    workthread_ =
        std::make_shared<BackgroundedWorkThread>(&bg_work_, [this]() { this->WorkCycle(); });
  }

  auto worker =
      std::make_shared<ExecutorConnectorWorker>(uri, muddle, timeout);
  bg_work_.Add(worker);
}

ExecutorInterface::Status ExecutorRpcClient::Execute(TxDigest const &hash, std::size_t slice,
                                                     LaneSet const &lanes)
{
  //  auto result = service_->Call(RPC_EXECUTOR, ExecutorRpcProtocol::EXECUTE, hash, slice, lanes);
  //  return result->As<Status>();

  auto result = client_->CallSpecificAddress(address_, RPC_EXECUTOR, ExecutorRpcProtocol::EXECUTE,
                                             hash, slice, lanes);
  return result->As<ExecutorInterface::Status>();
}

void ExecutorRpcClient::WorkCycle()
{
  if (!bg_work_.WorkCycle())
  {
    return;
  }

  if (bg_work_.CountSuccesses() > 0)
  {
    for (auto &successful_worker : bg_work_.GetSuccesses(1000))
    {
      if (successful_worker)
      {
        address_ = std::move(successful_worker->target_address);
        connections_++;
      }
    }
  }

  bg_work_.DiscardFailures();
  bg_work_.DiscardTimeouts();
}

}  // namespace ledger
}  // namespace fetch
