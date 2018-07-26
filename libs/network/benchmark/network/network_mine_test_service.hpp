#pragma once

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
class NetworkMineTestService
    : public service::ServiceServer<fetch::network::TCPServer>,
      public http::HTTPServer
{
public:
  NetworkMineTestService(fetch::network::NetworkManager tm, uint16_t tcpPort,
                         uint16_t httpPort)
      : ServiceServer(tcpPort, tm), HTTPServer(httpPort, tm)
  {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("Constructing test node service with TCP port: ",
                        tcpPort, " and HTTP port: ", httpPort);
    node_ = std::make_shared<T>(tm, tcpPort);

    httpInterface_ =
        std::make_shared<network_mine_test::HttpInterface<T>>(node_);
    networkMineTestProtocol_ =
        common::make_unique<protocols::NetworkMineTestProtocol<T>>(node_);

    this->Add(protocols::FetchProtocols::NETWORK_MINE_TEST,
              networkMineTestProtocol_.get());

    // Add middleware to the HTTP server - allow requests from any address,
    // and print requests to the terminal in colour
    this->AddMiddleware(http::middleware::AllowOrigin("*"));
    this->AddMiddleware(http::middleware::ColorLog);
    this->AddModule(*httpInterface_);
  }

private:
  std::shared_ptr<T>                                   node_;
  std::shared_ptr<network_mine_test::HttpInterface<T>> httpInterface_;
  std::unique_ptr<protocols::NetworkMineTestProtocol<T>>
      networkMineTestProtocol_;
};
}  // namespace network_mine_test
}  // namespace fetch
