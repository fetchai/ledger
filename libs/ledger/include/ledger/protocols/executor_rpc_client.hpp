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

#include <memory>
#include <cstddef>                                 // for size_t

#include "core/serializers/stl_types.hpp"
#include "core/service_ids.hpp"
#include "ledger/executor_interface.hpp"           // for ExecutorInterface
#include "ledger/protocols/executor_rpc_protocol.hpp"
#include "network/generics/atomic_state_machine.hpp"
#include "network/generics/backgrounded_work.hpp"  // for BackgroundedWork
#include "network/generics/future_timepoint.hpp"   // for FutureTimepoint
#include "network/generics/has_worker_thread.hpp"  // for HasWorkerThread
#include "network/muddle/muddle.hpp"               // for Muddle
#include "network/muddle/rpc/client.hpp"           // for Client
#include "network/muddle/rpc/server.hpp"
#include "network/service/service_client.hpp"
#include "network/tcp/tcp_client.hpp"
#include "meta/type_traits.hpp"                    // for ConstByteArray
#include "network/management/network_manager.hpp"  // for NetworkManager
#include "network/muddle/muddle_endpoint.hpp"      // for MuddleEndpoint
#include "network/service/promise.hpp"             // for PromiseState

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
  using Address         = Muddle::Address;  // == a crypto::Identity.identifier_
  using Uri             = Muddle::Uri;
  using PromiseState    = fetch::service::PromiseState;
  using FutureTimepoint = network::FutureTimepoint;

  std::shared_ptr<Client> client;

  explicit ExecutorRpcClient(NetworkManager const &tm, Muddle &muddle)
    : network_manager_(tm)
  {
    client_ = std::make_shared<Client>(muddle.AsEndpoint(), Muddle::Address(), SERVICE_EXECUTOR,
                                       CHANNEL_RPC);
  }

  void Connect(Muddle &muddle, Uri uri,
               std::chrono::milliseconds timeout = std::chrono::milliseconds(10000));

  Status Execute(TxDigest const &hash, std::size_t slice, LaneSet const &lanes) override;

  bool GetAddress(Address &address) const
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
    TIMEDOUT,
    FAILED,
  };

  ClientPtr      client_;
  NetworkManager network_manager_;
  ServicePtr     service_;

  using Worker                  = ExecutorConnectorWorker;
  using WorkerP                 = std::shared_ptr<Worker>;
  using BackgroundedWork        = network::BackgroundedWork<Worker>;
  using BackgroundedWorkThread  = network::HasWorkerThread<BackgroundedWork>;
  using BackgroundedWorkThreadP = std::shared_ptr<BackgroundedWorkThread>;

  Address                 address_;
  BackgroundedWork        bg_work_;
  BackgroundedWorkThreadP workthread_;
  size_t                  connections_ = 0;
};

}  // namespace ledger
}  // namespace fetch
