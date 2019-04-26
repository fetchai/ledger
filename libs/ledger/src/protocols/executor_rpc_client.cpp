//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

class ExecutorConnectorWorker
{
public:
  using Address         = ExecutorRpcClient::Address;
  using FutureTimepoint = ExecutorRpcClient::FutureTimepoint;
  using Muddle          = ExecutorRpcClient::Muddle;
  using PromiseState    = ExecutorRpcClient::PromiseState;
  using Uri             = ExecutorRpcClient::Uri;
  using State           = ExecutorRpcClient::State;
  using Client          = ExecutorRpcClient::Client;

  Address target_address;

  ExecutorConnectorWorker(Uri thepeer, Muddle &themuddle,
                          std::chrono::milliseconds thetimeout = std::chrono::milliseconds(10000))
    : state_machine_()
    , peer_(std::move(thepeer))
    , timeduration_(std::move(thetimeout))
    , muddle_(themuddle)
    , client_(std::make_shared<Client>("R:ExecCW", muddle_.AsEndpoint(), Muddle::Address(),
                                       SERVICE_EXECUTOR, CHANNEL_RPC))
  {
    state_machine_.Allow(State::CONNECTING, State::INITIAL)
        .Allow(State::SUCCESS, State::CONNECTING)
        .Allow(State::TIMEDOUT, State::CONNECTING)
        .Allow(State::FAILED, State::CONNECTING);

    FETCH_LOG_WARN(LOGGING_NAME, "INIT: ", peer_.ToString());
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
      state_machine_.Work();
    }
    catch (...)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Work threw.");
      throw;
    }
    auto r = state_machine_.Get();

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

  bool PossibleNewState(State &currentstate)
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
      bool connected_ = muddle_.UriToDirectAddress(peer_, target_address);
      if (!connected_)
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

private:
  network::AtomicStateMachine<ExecutorRpcClient::State> state_machine_;
  Uri                       peer_;
  std::chrono::milliseconds timeduration_;
  FutureTimepoint           timeout_;
  Muddle &                  muddle_;
  std::shared_ptr<Client>   client_;
  PendingConnectionCounter  counter_;
};

void ExecutorRpcClient::Connect(Muddle &muddle, Uri uri, std::chrono::milliseconds timeout)
{
  if (!workthread_)
  {
    workthread_ = std::make_shared<BackgroundedWorkThread>(&bg_work_, "BW:ExecRpcC",
                                                           [this]() { this->WorkCycle(); });
  }

  auto worker = std::make_shared<ExecutorConnectorWorker>(uri, muddle, timeout);
  bg_work_.Add(worker);
}

ExecutorInterface::Status ExecutorRpcClient::Execute(TxDigest const &hash, std::size_t slice,
                                                     LaneSet const &lanes)
{
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
