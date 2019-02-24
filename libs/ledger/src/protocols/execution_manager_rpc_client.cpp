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

#include <memory>
#include <utility>

#include "core/serializers/stl_types.hpp"
#include "core/service_ids.hpp"
#include "ledger/protocols/execution_manager_rpc_client.hpp"
#include "ledger/protocols/execution_manager_rpc_protocol.hpp"

namespace fetch {
namespace ledger {

using PendingConnectionCounter =
    network::AtomicInFlightCounter<network::AtomicCounterName::LOCAL_SERVICE_CONNECTIONS>;

class ExecutionManagerRpcConnectorWorker
{
public:
  using Address         = ExecutionManagerRpcClient::Address;
  using PromiseState    = ExecutionManagerRpcClient::PromiseState;
  using FutureTimepoint = ExecutionManagerRpcClient::FutureTimepoint;
  using Muddle          = ExecutionManagerRpcClient::Muddle;
  using MuddlePtr       = std::shared_ptr<Muddle>;
  using Uri             = ExecutionManagerRpcClient::Uri;
  using Client          = ExecutionManagerRpcClient::Client;

  Address target_address;

  ExecutionManagerRpcConnectorWorker(
      Uri thepeer, MuddlePtr themuddle,
      std::chrono::milliseconds thetimeout = std::chrono::milliseconds(10000))
    : peer_(std::move(thepeer))
    , timeduration_(std::move(thetimeout))
    , muddle_(std::move(themuddle))
  {
    client_ = std::make_shared<Client>("R:ExecMgrCW", muddle_->AsEndpoint(), Muddle::Address(),
                                       SERVICE_EXECUTOR, CHANNEL_RPC);
  }
  static constexpr char const *LOGGING_NAME = "MuddleLaneConnectorWorker";

  virtual ~ExecutionManagerRpcConnectorWorker()
  {
    counter_.Completed();
  }

  PromiseState Work()
  {
    if (!trying_)
    {
      trying_ = true;
      timeout_.Set(timeduration_);
      muddle_->AddPeer(peer_);
      return PromiseState::WAITING;
    }

    if (timeout_.IsDue())
    {
      return PromiseState::TIMEDOUT;
    }

    bool connected = muddle_->UriToDirectAddress(peer_, target_address);
    if (!connected)
    {
      return PromiseState::WAITING;
    }

    return PromiseState::SUCCESS;
  }

private:
  std::shared_ptr<Client>   client_;
  Uri                       peer_;
  std::chrono::milliseconds timeduration_;
  PendingConnectionCounter  counter_;
  MuddlePtr                 muddle_;
  FutureTimepoint           timeout_;
  bool                      trying_ = false;
};

void ExecutionManagerRpcClient::WorkCycle(void)
{
  if (!bg_work_.WorkCycle())
  {
    return;
  }
  for (auto &successful_worker : bg_work_.GetSuccesses(1000))
  {
    address_     = std::move(successful_worker->target_address);
    connections_ = 1;
  }
  bg_work_.DiscardFailures();
  bg_work_.DiscardTimeouts();
}

ExecutionManagerRpcClient::ExecutionManagerRpcClient(NetworkManager const &network_manager)
  : network_manager_(network_manager)
{
  muddle_ = Muddle::CreateMuddle(Muddle::CreateNetworkId("EXEM"), network_manager_);
  client_ = std::make_shared<Client>("R:ExecMgr", muddle_->AsEndpoint(), Muddle::Address(),
                                     SERVICE_LANE, CHANNEL_RPC);
  muddle_->Start({});
}

void ExecutionManagerRpcClient::AddConnection(const Uri &                      uri,
                                              const std::chrono::milliseconds &timeout)
{
  if (!workthread_)
  {
    workthread_ = std::make_shared<BackgroundedWorkThread>(&bg_work_, "BW:ExcMgrRpc",
                                                           [this]() { this->WorkCycle(); });
  }

  auto worker = std::make_shared<ExecutionManagerRpcConnectorWorker>(
      uri, muddle_, std::chrono::milliseconds(timeout));
  bg_work_.Add(worker);
}

ExecutionManagerRpcClient::ScheduleStatus ExecutionManagerRpcClient::Execute(
    Block::Body const &block)
{
  auto result = client_->CallSpecificAddress(address_, RPC_EXECUTION_MANAGER,
                                             ExecutionManagerRpcProtocol::EXECUTE, block);
  return result->As<ScheduleStatus>();
}

ExecutionManagerInterface::BlockHash ExecutionManagerRpcClient::LastProcessedBlock()
{
  auto result = client_->CallSpecificAddress(address_, RPC_EXECUTION_MANAGER,
                                             ExecutionManagerRpcProtocol::LAST_PROCESSED_BLOCK);
  return result->As<BlockHash>();
}

ExecutionManagerRpcClient::State ExecutionManagerRpcClient::GetState()
{
  auto result = client_->CallSpecificAddress(address_, RPC_EXECUTION_MANAGER,
                                             ExecutionManagerRpcProtocol::GET_STATE);
  return result->As<State>();
}

bool ExecutionManagerRpcClient::Abort()
{
  auto result = client_->CallSpecificAddress(address_, RPC_EXECUTION_MANAGER,
                                             ExecutionManagerRpcProtocol::ABORT);
  return result->As<bool>();
}

void ExecutionManagerRpcClient::SetLastProcessedBlock(ExecutionManagerInterface::BlockHash hash)
{
  auto result = client_->CallSpecificAddress(
      address_, RPC_EXECUTION_MANAGER, ExecutionManagerRpcProtocol::SET_LAST_PROCESSED_BLOCK, hash);
  result->Wait();
}

}  // namespace ledger
}  // namespace fetch
