#include"service/server.hpp"
#include"network/tcp_server.hpp"
#include"protocols/discovery_protocol.hpp"
#include"commandline/parameter_parser.hpp"
#include"commandline/vt100.hpp"
#include"http/server.hpp"
#include<vector>
#include<memory> 

using namespace fetch::commandline;
using namespace fetch::protocols;
enum FetchProtocols 
{
  DISCOVERY = 1
};

class FetchService  
{
public:
  FetchService(uint16_t port, std::string const&pk) :   
    thread_manager_( new fetch::network::ThreadManager(8) ),
    service_(1337+port, thread_manager_),
    http_server_(8080+port, thread_manager_)
  {
    std::cout << "Listening for peers on " << (1337+port) << ", clients on " << (8080 + port ) << std::endl;
      
    details_.public_key = pk;
    discovery_ =  new DiscoveryProtocol(thread_manager_, FetchProtocols::DISCOVERY, details_);
    
    service_.Add(FetchProtocols::DISCOVERY, discovery_ );

    // Setting callback to resolve IP
    discovery_->SetClientIPCallback([this](uint64_t const &n) -> std::string {
        return service_.GetAddress(n);        
      });

    http_server_.AddMiddleware( [](fetch::http::HTTPResponse &res, fetch::http::HTTPRequest const &req) {
        res.header().Add("Access-Control-Allow-Origin", "*");
      });
    
    
    // HTTP
    http_server_.AddMiddleware( [](fetch::http::HTTPResponse &res, fetch::http::HTTPRequest const &req) {
        using namespace fetch::commandline::VT100;
        std::string color = "";
        
        switch(res.status().code / 100 ) {
        case 0:
          color = GetColor(9,9);
          break;
        case 1:
          color = GetColor(4,9);
          break;          
        case 2:
          color = GetColor(3,9);          
          break;          
        case 3:
          color = GetColor(5,9);             
          break;          
        case 4:
          color = GetColor(1,9);          
          break;          
        case 5:
          color = GetColor(6,9);            
          break;          
        };
        
        std::cout << "[ " << color << res.status().explanation << DefaultAttributes() << " ] " << req.uri();
        std::cout << ", " << GetColor(5,9) << res.mime_type().type <<   DefaultAttributes() << std::endl;      
    });


    auto http_bootstrap = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
        std::cout << "Connecting to " << params["ip"] << " on port " << params["port"] << std::endl;
        
        discovery_->Bootstrap( params["ip"] , params["port"].AsInt() );
        return fetch::http::HTTPResponse("{\"status\":\"ok\"}");
        
    };
    
    http_server_.AddView("/bootstrap/(ip=\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/(port=\\d+)",  http_bootstrap);
    http_server_.AddView("/connect-to/(ip=\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/(port=\\d+)",  http_bootstrap);


    auto list_incoming = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      std::string response = "{\"peers\": [";
      
      response += "]}";      
      return fetch::http::HTTPResponse(response);
    }
    
    http_server_.AddView("/list/incoming",  list_incoming);     
    
  }

  ~FetchService() 
  {
    std::cout << "Killing fetch service";    
    delete discovery_;
  }

  void Bootstrap(std::string const &address, uint16_t const &port) 
  {
    discovery_->Bootstrap( address, port );
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
  
  DiscoveryProtocol *discovery_ = nullptr;
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
