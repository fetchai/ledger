
#include"service/server.hpp"
#include"network/tcp_server.hpp"
#include"protocols/node_discovery.hpp"
#include"commandline/parameter_parser.hpp"
#include"commandline/vt100.hpp"
#include<vector>
#include<memory>

using namespace fetch::commandline;
using namespace fetch::protocols;
enum FetchProtocols {
  DISCOVERY = 1
};

class FetchService  {
public:
  FetchService(uint16_t port, std::string const&pk) :
    thread_manager_(8),
    service_(port, &thread_manager_)
  {
    details_.public_key = pk;
    discovery_ =  new DiscoveryProtocol(&thread_manager_, FetchProtocols::DISCOVERY, details_);
    
    service_.Add(FetchProtocols::DISCOVERY, discovery_ );
  }

  ~FetchService() {
    delete discovery_;
  }

  void Bootstrap(std::string const &address, uint16_t const &port) {
    discovery_->Bootstrap( address, port );
  }

  void Start() 
  {
    thread_manager_.Start();
  }

  void Stop() 
  {
    thread_manager_.Stop();
  }
  
private:
  fetch::network::ThreadManager thread_manager_;    
  fetch::service::ServiceServer< fetch::network::TCPServer > service_;
  
  DiscoveryProtocol *discovery_ = nullptr;
  NodeDetails details_;
};


int main(int argc, char const** argv) {

  ParamsParser params;
  params.Parse(argc, argv);
 
  if(params.arg_size() < 3) {
    std::cout << "usage: " << argv[0] << " [port] [info] [[bootstrap_host] [bootstrap_port]]" << std::endl;
    exit(-1);
  }
  
  uint16_t  my_port = params.GetArg<uint16_t>(1);
  std::string  info = params.GetArg(2);    
  std::cout << "Listening on " << my_port << std::endl;
  FetchService service(my_port, info);
  service.Start();
  
  std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );  
  
  if(params.arg_size() >= 5) {
    std::string host = params.GetArg(3);
    uint16_t port = params.GetArg<uint16_t>(4);
    std::cout << "Bootstrapping through " << host << " " << port << std::endl;
    service.Bootstrap(host, port);
  }
  
  
  std::cout << "Ctrl-C to stop" << std::endl;
  while(true) {
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
  }
  return 0;
}
