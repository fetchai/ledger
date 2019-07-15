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

#include "core/future_timepoint.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/service_ids.hpp"
#include "ledger/executor_interface.hpp"
#include "ledger/protocols/executor_rpc_protocol.hpp"
#include "network/generics/backgrounded_work.hpp"
#include "network/generics/has_worker_thread.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/service/service_client.hpp"
#include "network/tcp/tcp_client.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace fetch {
namespace ledger {

class ExecutorConnectorWorker;

class ExecutorRpcClient : public ExecutorInterface
{
public:
  using MuddleEp        = muddle::MuddleEndpoint;
  using Muddle          = muddle::Muddle;
  using Client          = muddle::rpc::Client;
  using ClientPtr       = std::shared_ptr<Client>;
  using ServicePtr      = std::unique_ptr<service::ServiceClient>;
  using NetworkManager  = fetch::network::NetworkManager;
  using ConstByteArray  = byte_array::ConstByteArray;
  using Uri             = Muddle::Uri;
  using PromiseState    = fetch::service::PromiseState;
  using FutureTimepoint = core::FutureTimepoint;

  std::shared_ptr<Client> client;

  explicit ExecutorRpcClient(Muddle &muddle)
    : client_(
          std::make_shared<Client>("R:Exec", muddle.AsEndpoint(), SERVICE_EXECUTOR, CHANNEL_RPC))
  {}

  void Connect(Muddle &muddle, Uri uri,
               std::chrono::milliseconds timeout = std::chrono::milliseconds(10000));

  Result Execute(Digest const &digest, BlockIndex block, SliceIndex slice,
                 BitVector const &shards) override;
  void   SettleFees(Address const &miner, TokenAmount amount, uint32_t log2_num_lanes) override;

  bool GetAddress(Muddle::Address &address) const
  {
    if (!connections_)
    {
      return false;
    }
    address = address_;
    return true;
  }

  std::size_t connections() const
  {
    return connections_;
  }

  ClientPtr GetClient()
  {
    return client_;
  }

private:
  void WorkCycle();

  friend class ExecutorConnectorWorker;

  enum class State
  {
    INITIAL = 0,
    CONNECTING,
    SUCCESS,
    TIMEDOUT
  };

  ClientPtr  client_;
  ServicePtr service_;

  using Worker                  = ExecutorConnectorWorker;
  using WorkerP                 = std::shared_ptr<Worker>;
  using BackgroundedWork        = network::BackgroundedWork<Worker>;
  using BackgroundedWorkThread  = network::HasWorkerThread<BackgroundedWork>;
  using BackgroundedWorkThreadP = std::shared_ptr<BackgroundedWorkThread>;

  Muddle::Address         address_;
  BackgroundedWork        bg_work_;
  BackgroundedWorkThreadP workthread_;
  std::size_t             connections_ = 0;
};

}  // namespace ledger
}  // namespace fetch
