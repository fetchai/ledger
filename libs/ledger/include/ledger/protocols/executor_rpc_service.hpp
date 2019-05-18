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

#include "core/service_ids.hpp"
#include "ledger/executor.hpp"
#include "ledger/protocols/executor_rpc_protocol.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/service/server.hpp"
#include "network/tcp/tcp_server.hpp"

namespace fetch {
namespace ledger {

class ExecutorRpcService
{
public:
  using StorageUnitPtr         = Executor::StorageUnitPtr;
  using Muddle                 = muddle::Muddle;
  using MuddlePtr              = std::shared_ptr<Muddle>;
  using Identity               = crypto::Identity;
  using Server                 = fetch::muddle::rpc::Server;
  using ServerPtr              = std::shared_ptr<Server>;
  using CertificatePtr         = Muddle::CertificatePtr;
  using ExecutorRpcProtocolPtr = std::shared_ptr<ExecutorRpcProtocol>;

  static constexpr char const *LOGGING_NAME = "ExecutorRpcService";

  // Construction / Destruction
  ExecutorRpcService(uint16_t port, StorageUnitPtr storage, MuddlePtr muddle)
    : executor_(std::move(storage))
    , port_(port)
    , muddle_(std::move(muddle))
    , identity_(muddle_->identity())
    , server_(std::make_shared<Server>(muddle_->AsEndpoint(), SERVICE_EXECUTOR, CHANNEL_RPC))
    , protocol_(std::make_shared<ExecutorRpcProtocol>(executor_))
  {
    server_->Add(RPC_EXECUTOR, protocol_.get());
  }
  ~ExecutorRpcService()
  {
    muddle_->Stop();
    protocol_.reset();
  }

  void Start()
  {
    muddle_->Start({port_});
    FETCH_LOG_INFO(LOGGING_NAME, "Establishing ExecutorRpcService on rpc://127.0.0.1:", port_,
                   " ID: ", byte_array::ToBase64(identity_.identifier()));
  }

  void Stop()
  {
    muddle_->Stop();
  }

private:
  Executor               executor_;
  uint16_t               port_;
  MuddlePtr              muddle_;
  Identity               identity_;
  ServerPtr              server_;
  ExecutorRpcProtocolPtr protocol_;
};

}  // namespace ledger
}  // namespace fetch
