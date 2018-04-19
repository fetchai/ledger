#ifndef NETWORK_BENCHMARK_SERVICE_HPP
#define NETWORK_BENCHMARK_SERVICE_HPP

#include<memory>
#include"service/server.hpp"
#include"http/server.hpp"
#include"./protocols/network_benchmark.hpp"
#include"./node.hpp"
#include"./http_interface.hpp"

// Fetch node service connects the desired protocols to the OEF API

namespace fetch
{
namespace network_benchmark
{

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

class NetworkBenchmarkService : public service::ServiceServer< fetch::network::TCPServer >, public http::HTTPServer
{
public:
  NetworkBenchmarkService(fetch::network::ThreadManager *tm, uint16_t tcpPort, uint16_t httpPort, int seed) :
    ServiceServer(tcpPort, tm),
    HTTPServer(httpPort, tm)
  {

    fetch::logger.Debug("Constructing test node service with TCP port: ", tcpPort, " and HTTP port: ", httpPort);
    node_                     = std::make_shared<network_benchmark::Node>(tm, seed);

    httpInterface_            = std::make_shared<network_benchmark::HttpInterface>(node_);
    networkBenchmarkProtocol_ = make_unique<protocols::NetworkBenchmarkProtocol<network_benchmark::Node>>(node_);

    this->Add(protocols::FetchProtocols::NETWORK_BENCHMARK, networkBenchmarkProtocol_.get());

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
  std::shared_ptr<network_benchmark::Node>                                      node_;
  std::shared_ptr<network_benchmark::HttpInterface>                             httpInterface_;
  std::unique_ptr<protocols::NetworkBenchmarkProtocol<network_benchmark::Node>> networkBenchmarkProtocol_;
};
}
}

#endif
