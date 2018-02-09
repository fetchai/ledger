#include"service/server.hpp"
#include"network/tcp_server.hpp"
#include"protocols.hpp"
#include"commandline/parameter_parser.hpp"

#include"http/server.hpp"
#include"http/middleware/allow_origin.hpp"
#include"http/middleware/color_log.hpp"
#include<vector>
#include<memory> 

using namespace fetch::commandline;
using namespace fetch::protocols;

class FetchService  
{
public:
  FetchService(uint16_t port, std::string const&pk) :   
    thread_manager_( new fetch::network::ThreadManager(8) ),
    service_(1337+port, thread_manager_),
    http_server_(8080+port, thread_manager_)
  {
    
    std::cout << "Listening for peers on " << (1337+port) << ", clients on " << (8080 + port ) << std::endl;
      
    details_.public_key() = pk;
    details_.default_port() = 1337+port;
    details_.default_http_port() = 8080 + port;

    // Creating a service contiaing the swarm protocol
    swarm_ =  new SwarmProtocol(thread_manager_, FetchProtocols::SWARM, details_);
    service_.Add(FetchProtocols::SWARM, swarm_ );

    // Setting callback to resolve IP
    swarm_->SetClientIPCallback([this](uint64_t const &n) -> std::string {
        return service_.GetAddress(n);        
      });

    // Creating a http server based on the swarm protocol
    http_server_.AddMiddleware( fetch::http::middleware::AllowOrigin("*") );       
    http_server_.AddMiddleware( fetch::http::middleware::ColorLog);
    http_server_.AddModule(*swarm_);
    
  }

  ~FetchService() 
  {
    std::cout << "Killing fetch service";    
    delete swarm_;
  }

  void Bootstrap(std::string const &address, uint16_t const &port) 
  {
    swarm_->Bootstrap( address, port );
  }

  void Start() 
  {
    thread_manager_->Start();
  }

  void Stop() 
  
  {
    thread_manager_->Stop();
  }
  
private:
  fetch::network::ThreadManager *thread_manager_;    
  fetch::service::ServiceServer< fetch::network::TCPServer > service_;
  fetch::http::HTTPServer http_server_;  
  
  SwarmProtocol *swarm_ = nullptr;
  NodeDetails details_;
};


int main(int argc, char const** argv) 
{

  ParamsParser params;
  params.Parse(argc, argv);
 
  if(params.arg_size() < 3) 
  {
    std::cout << "usage: " << argv[0] << " [port offset] [info] [[bootstrap_host] [bootstrap_port]]" << std::endl;
    exit(-1);
  }
  
  uint16_t  my_port = params.GetArg<uint16_t>(1);
  std::string  info = params.GetArg(2);    
  std::cout << "Listening on " << my_port << std::endl;
  FetchService service(my_port, info);
  service.Start();
  
  std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );  
  
  if(params.arg_size() >= 5) 
  {
    std::string host = params.GetArg(3);
    uint16_t port = params.GetArg<uint16_t>(4);
    std::cout << "Bootstrapping through " << host << " " << port << std::endl;
    service.Bootstrap(host, port);
  }
  
  
  std::cout << "Ctrl-C to stop" << std::endl;
  while(true) 
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
  }
  return 0;
}
