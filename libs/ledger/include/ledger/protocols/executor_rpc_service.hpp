#pragma once
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

class ExecutorRpcService  //: public service::ServiceServer<network::TCPServer>
{
public:
  using Resources              = Executor::Resources;
  using Muddle                 = muddle::Muddle;
  using MuddlePtr              = std::shared_ptr<Muddle>;
  using Identity               = crypto::Identity;
  using Server                 = fetch::muddle::rpc::Server;
  using ServerPtr              = std::shared_ptr<Server>;
  using CertificatePtr         = Muddle::CertificatePtr;  // == std::unique_ptr<crypto::Prover>;
  using ExecutorRpcProtocolPtr = std::shared_ptr<ExecutorRpcProtocol>;

  static constexpr char const *LOGGING_NAME = "ExecutorRpcService";

  // Construction / Destruction
  ExecutorRpcService(uint16_t port, fetch::network::NetworkManager tm, Resources resources)
    //: ServiceServer(port, network_manager)
    : executor_(std::move(resources))
  {
    port_                            = port;
    crypto::ECDSASigner *certificate = new crypto::ECDSASigner();
    certificate->GenerateKeys();

    std::unique_ptr<crypto::Prover> certificate_;
    certificate_.reset(certificate);

    identity_ = certificate_->identity();
    muddle_   = std::make_shared<Muddle>(std::move(certificate_), tm);
    server_   = std::make_shared<Server>(muddle_->AsEndpoint(), SERVICE_EXECUTOR, CHANNEL_RPC);
    protocol_ = std::make_shared<ExecutorRpcProtocol>(executor_);

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
  Identity               identity_;
  uint16_t               port_;
  ServerPtr              server_;
  MuddlePtr              muddle_;  ///< The muddle networking service
  Executor               executor_;
  ExecutorRpcProtocolPtr protocol_;
};

}  // namespace ledger
}  // namespace fetch
