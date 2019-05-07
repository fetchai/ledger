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
#include "core/state_machine.hpp"

#include <memory>

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
    : state_machine_{std::make_shared<core::StateMachine<State>>("ExecutorConnectorWorker",
                                                                 State::INITIAL)}
    , peer_(std::move(thepeer))
    , timeduration_(std::move(thetimeout))
    , muddle_(themuddle)
    , client_(std::make_shared<Client>("R:ExecCW", muddle_.AsEndpoint(), Muddle::Address(),
                                       SERVICE_EXECUTOR, CHANNEL_RPC))
  {
    state_machine_->RegisterHandler(State::INITIAL, this, &ExecutorConnectorWorker::OnInitial);
    state_machine_->RegisterHandler(State::CONNECTING, this,
                                    &ExecutorConnectorWorker::OnConnecting);
    state_machine_->RegisterHandler(State::SUCCESS, this, &ExecutorConnectorWorker::OnSuccess);
    state_machine_->RegisterHandler(State::TIMEDOUT, this, &ExecutorConnectorWorker::OnTimedOut);

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
      if (state_machine_->IsReadyToExecute())
      {
        state_machine_->Execute();
      }
    }
    catch (...)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Work threw.");
      throw;
    }
    auto r = state_machine_->state();

    switch (r)
    {
    case State::TIMEDOUT:
      return PromiseState::TIMEDOUT;
    case State::SUCCESS:
      return PromiseState::SUCCESS;
    default:
      return PromiseState::WAITING;
    }
  }

private:
  State OnInitial()
  {
    timeout_.Set(timeduration_);
    muddle_.AddPeer(peer_);

    return State::CONNECTING;
  }

  State OnConnecting()
  {
    bool connected_ = muddle_.UriToDirectAddress(peer_, target_address);

    if (connected_)
    {
      return State::SUCCESS;
    }

    if (timeout_.IsDue())
    {
      return State::TIMEDOUT;
    }

    return State::CONNECTING;
  }

  State OnSuccess()
  {
    return State::SUCCESS;
  }

  State OnTimedOut()
  {
    return State::TIMEDOUT;
  }

  std::shared_ptr<core::StateMachine<State>> state_machine_;
  Uri                                        peer_;
  std::chrono::milliseconds                  timeduration_;
  FutureTimepoint                            timeout_;
  Muddle &                                   muddle_;
  std::shared_ptr<Client>                    client_;
  PendingConnectionCounter                   counter_;
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
