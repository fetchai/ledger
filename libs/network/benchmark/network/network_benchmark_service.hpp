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

#include "core/logger.hpp"
#include "helper_functions.hpp"
#include "http/server.hpp"
#include "http_interface.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include "network/service/server.hpp"
#include "node_basic.hpp"
#include "protocols/network_benchmark.hpp"
#include <memory>

namespace fetch {
namespace network_benchmark {

template <typename T>
class NetworkBenchmarkService : public http::HTTPServer
{
public:
  static constexpr char const *LOGGING_NAME = "NetworkBenchmarkService";

  TServerPtr server;

  NetworkBenchmarkService(fetch::network::NetworkManager const &tm, uint16_t tcpPort,
                          uint16_t httpPort)
    : HTTPServer(tm)
    , http_port_{httpPort}
  {
    LOG_STACK_TRACE_POINT;
    FETCH_LOG_DEBUG(LOGGING_NAME, "Constructing test node service with TCP port: ", tcpPort,
                    " and HTTP port: ", httpPort);
    node_ = std::make_shared<T>();

    server = MuddleTestServer::CreateTestServer(tcpPort);

    httpInterface_            = std::make_shared<network_benchmark::HttpInterface<T>>(node_);
    networkBenchmarkProtocol_ = std::make_unique<protocols::NetworkBenchmarkProtocol<T>>(node_);

    server->Add(protocols::FetchProtocols::NETWORK_BENCHMARK, networkBenchmarkProtocol_.get());

    // Add middleware to the HTTP server - allow requests from any address,
    // and print requests to the terminal in colour
    this->AddMiddleware(http::middleware::AllowOrigin("*"));
    this->AddMiddleware(http::middleware::ColorLog);
    this->AddModule(*httpInterface_);
  }

  void Start()
  {
    HTTPServer::Start(http_port_);
  }

  void Stop()
  {
    HTTPServer::Stop();
  }

private:
  uint16_t                                                http_port_;
  std::shared_ptr<T>                                      node_;
  std::shared_ptr<network_benchmark::HttpInterface<T>>    httpInterface_;
  std::unique_ptr<protocols::NetworkBenchmarkProtocol<T>> networkBenchmarkProtocol_;
};
}  // namespace network_benchmark
}  // namespace fetch
