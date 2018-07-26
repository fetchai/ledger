#include "core/commandline/parameter_parser.hpp"
#include "core/commandline/vt100.hpp"
#include "http/server.hpp"
#include "network/protocols/discovery_protocol.hpp"
#include "network/service/server.hpp"
#include "network/tcp/tcp_server.hpp"
#include <memory>
#include <vector>

using namespace fetch::commandline;
using namespace fetch::protocols;
enum FetchProtocols
{
  DISCOVERY = 1
};

class FetchService
{
public:
  FetchService(uint16_t port, std::string const &pk)
      : network_manager_(new fetch::network::NetworkManager(8))
      , service_(port, network_manager_)
  {
    details_.public_key = pk;
    discovery_          = new DiscoveryProtocol(network_manager_,
                                       FetchProtocols::DISCOVERY, details_);

    service_.Add(FetchProtocols::DISCOVERY, discovery_);

    // Setting callback to resolve IP
    discovery_->SetClientIPCallback([this](uint64_t const &n) -> std::string {
      return service_.GetAddress(n);
    });
  }

  ~FetchService()
  {
    std::cout << "Killing fetch service";
    delete discovery_;
  }

  void Bootstrap(std::string const &address, uint16_t const &port)
  {
    discovery_->Bootstrap(address, port);
  }

  void Start() { network_manager_->Start(); }

  void Stop() { network_manager_->Stop(); }

private:
  fetch::network::NetworkManager *                         network_manager_;
  fetch::service::ServiceServer<fetch::network::TCPServer> service_;

  DiscoveryProtocol *discovery_ = nullptr;
  NodeDetails        details_;
};

int main(int argc, char const **argv)
{

  ParamsParser params;
  params.Parse(argc, argv);

  if (params.arg_size() < 3)
  {
    std::cout << "usage: " << argv[0]
              << " [port] [info] [[bootstrap_host] [bootstrap_port]]"
              << std::endl;
    exit(-1);
  }

  uint16_t    my_port = params.GetArg<uint16_t>(1);
  std::string info    = params.GetArg(2);
  std::cout << "Listening on " << my_port << std::endl;
  FetchService service(my_port, info);
  service.Start();

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  if (params.arg_size() >= 5)
  {
    std::string host = params.GetArg(3);
    uint16_t    port = params.GetArg<uint16_t>(4);
    std::cout << "Bootstrapping through " << host << " " << port << std::endl;
    service.Bootstrap(host, port);
  }

  std::cout << "Ctrl-C to stop" << std::endl;
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  return 0;
}
