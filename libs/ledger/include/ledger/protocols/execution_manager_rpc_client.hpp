#pragma once
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

#include "ledger/execution_manager_interface.hpp"
#include "network/generics/backgrounded_work.hpp"
#include "network/generics/has_worker_thread.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/service/service_client.hpp"
#include "network/tcp/tcp_client.hpp"

#include <memory>

namespace fetch {
namespace ledger {

class ExecutionManagerRpcConnectorWorker;

class ExecutionManagerRpcClient : public ExecutionManagerInterface
{
public:
  using ConstByteArray  = byte_array::ConstByteArray;
  using NetworkManager  = network::NetworkManager;
  using PromiseState    = fetch::service::PromiseState;
  using Promise         = service::Promise;
  using FutureTimepoint = core::FutureTimepoint;
  using MuddleEp        = muddle::MuddleEndpoint;
  using Muddle          = muddle::Muddle;
  using MuddlePtr       = std::shared_ptr<Muddle>;
  using Uri             = Muddle::Uri;
  using Address         = Muddle::Address;
  using Client          = muddle::rpc::Client;
  using ClientPtr       = std::shared_ptr<Client>;

  // Construction / Destruction
  ExecutionManagerRpcClient(NetworkManager const &network_manager);
  ~ExecutionManagerRpcClient() override = default;

  void AddConnection(const Uri &uri, const std::chrono::milliseconds &timeout);

  /// @name Execution Manager Interface
  /// @{
  ScheduleStatus Execute(Block::Body const &block) override;
  void           SetLastProcessedBlock(BlockHash hash) override;
  BlockHash      LastProcessedBlock() override;
  State          GetState() override;
  bool           Abort() override;
  /// @}

  void WorkCycle();

  std::size_t connections() const
  {
    return connections_;
  }

private:
  friend class ExecutionManagerRpcConnectorWorker;
  using Worker                    = ExecutionManagerRpcConnectorWorker;
  using WorkerPtr                 = std::shared_ptr<Worker>;
  using BackgroundedWork          = network::BackgroundedWork<Worker>;
  using BackgroundedWorkThread    = network::HasWorkerThread<BackgroundedWork>;
  using BackgroundedWorkThreadPtr = std::shared_ptr<BackgroundedWorkThread>;

  NetworkManager            network_manager_;
  BackgroundedWork          bg_work_;
  BackgroundedWorkThreadPtr workthread_;
  MuddlePtr                 muddle_;
  Address                   address_;
  size_t                    connections_ = 0;
  ClientPtr                 client_;
};

}  // namespace ledger
}  // namespace fetch
