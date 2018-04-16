#ifndef NETWORK_TEST_SERVICE_HPP
#define NETWORK_TEST_SERVICE_HPP

#include<memory>
#include"service/server.hpp"
#include"http/server.hpp"
#include"protocols/network_test.hpp"
#include"./node.hpp"
#include"./http_interface.hpp"

// Fetch node service connects the desired protocols to the OEF API

namespace fetch
{
namespace network_test
{

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

class NetworkTestService : public service::ServiceServer< fetch::network::TCPServer >, public http::HTTPServer
{
public:
  NetworkTestService(fetch::network::ThreadManager *tm, uint16_t tcpPort, uint16_t httpPort, int seed) :
    ServiceServer(tcpPort, tm),
    HTTPServer(httpPort, tm)
  {

    fetch::logger.Debug("Constructing test node service with TCP port: ", tcpPort, " and HTTP port: ", httpPort);
    node_                = std::make_shared<network_test::Node>(tm, seed);

    httpInterface_       =      std::make_shared<network_test::HttpInterface>(node_);
    networkTestProtocol_ =      make_unique<protocols::NetworkTestProtocol<network_test::Node>>(node_);

    this->Add(protocols::FetchProtocols::NETWORK_TEST, networkTestProtocol_.get());

    // Add middleware to the HTTP server - allow requests from any address, and print requests to the terminal in colour
    this->AddMiddleware(http::middleware::AllowOrigin("*"));
    this->AddMiddleware(http::middleware::ColorLog);
    this->AddModule(*httpInterface_);
  }

  void Start()
  {
    node_->Start();
  }

private:
  std::shared_ptr<network_test::Node>                                 node_;
  std::shared_ptr<network_test::HttpInterface>                        httpInterface_;
  std::unique_ptr<protocols::NetworkTestProtocol<network_test::Node>> networkTestProtocol_;
};
}
}

#endif
