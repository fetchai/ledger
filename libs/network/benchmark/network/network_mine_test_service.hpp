#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "../tests/include/helper_functions.hpp"
#include "./mine_test_http_interface.hpp"
#include "./protocols/fetch_protocols.hpp"
#include "./protocols/network_mine_test.hpp"
#include "core/logger.hpp"
#include "http/server.hpp"
#include "network/service/server.hpp"
#include <memory>

namespace fetch {
namespace network_mine_test {

template <typename T>
class NetworkMineTestService : public service::ServiceServer<fetch::network::TCPServer>,
                               public http::HTTPServer
{
public:
  NetworkMineTestService(fetch::network::NetworkManager tm, uint16_t tcpPort, uint16_t httpPort)
    : ServiceServer(tcpPort, tm), HTTPServer(httpPort, tm)
  {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("Constructing test node service with TCP port: ", tcpPort,
                        " and HTTP port: ", httpPort);
    node_ = std::make_shared<T>(tm, tcpPort);

    httpInterface_           = std::make_shared<network_mine_test::HttpInterface<T>>(node_);
    networkMineTestProtocol_ = std::make_unique<protocols::NetworkMineTestProtocol<T>>(node_);

    this->Add(protocols::FetchProtocols::NETWORK_MINE_TEST, networkMineTestProtocol_.get());

    // Add middleware to the HTTP server - allow requests from any address,
    // and print requests to the terminal in colour
    this->AddMiddleware(http::middleware::AllowOrigin("*"));
    this->AddMiddleware(http::middleware::ColorLog);
    this->AddModule(*httpInterface_);
  }

private:
  std::shared_ptr<T>                                     node_;
  std::shared_ptr<network_mine_test::HttpInterface<T>>   httpInterface_;
  std::unique_ptr<protocols::NetworkMineTestProtocol<T>> networkMineTestProtocol_;
};
}  // namespace network_mine_test
}  // namespace fetch
