#ifndef NETWORK_BENCHMARK_SERVICE_HPP
#define NETWORK_BENCHMARK_SERVICE_HPP

#include<memory>
#include"core/logger.hpp"
#include"network/service/server.hpp"
#include"http/server.hpp"
#include"./protocols/network_benchmark.hpp"
#include"./node_basic.hpp"
#include"./http_interface.hpp"
#include"../tests/include/helper_functions.hpp"


namespace fetch
{
namespace network_benchmark
{

template <typename T>
class NetworkBenchmarkService :
  public service::ServiceServer< fetch::network::TCPServer >, public http::HTTPServer
{
public:
  NetworkBenchmarkService(fetch::network::ThreadManager tm, uint16_t tcpPort, uint16_t httpPort) :
    ServiceServer(tcpPort, tm),
    HTTPServer(httpPort, tm)
  {
    LOG_STACK_TRACE_POINT ;
    fetch::logger.Debug("Constructing test node service with TCP port: ",
        tcpPort, " and HTTP port: ", httpPort);
    node_                     = std::make_shared<T>(tm);

    httpInterface_            = std::make_shared<network_benchmark::HttpInterface<T>>(node_);
    networkBenchmarkProtocol_ = common::make_unique<protocols::NetworkBenchmarkProtocol<T>>(node_);

    this->Add(protocols::FetchProtocols::NETWORK_BENCHMARK, networkBenchmarkProtocol_.get());

    // Add middleware to the HTTP server - allow requests from any address,
    // and print requests to the terminal in colour
    this->AddMiddleware(http::middleware::AllowOrigin("*"));
    this->AddMiddleware(http::middleware::ColorLog);
    this->AddModule(*httpInterface_);
  }

  void Start()
  {
    node_->Start();
  }

private:
  std::shared_ptr<T>                                         node_;
  std::shared_ptr<network_benchmark::HttpInterface<T>>       httpInterface_;
  std::unique_ptr<protocols::NetworkBenchmarkProtocol<T>>    networkBenchmarkProtocol_;
};
}
}

#endif
