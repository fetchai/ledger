#include"service/server.hpp"
#include"network/tcp_server.hpp"
#include"protocols/swarm.hpp"
#include"commandline/parameter_parser.hpp"
#include"commandline/vt100.hpp"
#include"http/server.hpp"
#include<vector>
#include<memory> 

using namespace fetch::commandline;
using namespace fetch::protocols;
enum FetchProtocols 
{
  SWARM = 1
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
      
    details_.public_key() = pk;
    details_.default_port() = 1337+port;
    details_.default_http_port() = 8080 + port;
    
    swarm_ =  new SwarmProtocol(thread_manager_, FetchProtocols::SWARM, details_);
    
    service_.Add(FetchProtocols::SWARM, swarm_ );

    // Setting callback to resolve IP
    swarm_->SetClientIPCallback([this](uint64_t const &n) -> std::string {
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
        
        swarm_->Bootstrap( params["ip"] , params["port"].AsInt() );
        return fetch::http::HTTPResponse("{\"status\":\"ok\"}");
        
    };
    
    http_server_.AddView("/bootstrap/(ip=\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/(port=\\d+)",  http_bootstrap);
    http_server_.AddView("/connect-to/(ip=\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/(port=\\d+)",  http_bootstrap);


    auto list_outgoing =  [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      std::stringstream response;
      
      response << "{\"outgoing\": [";  
      
      swarm_->with_server_details_do([&response](std::map< uint64_t, NodeDetails > const &peers) {
          bool first = true;          
          for(auto &ppair: peers)
          {
            auto &p = ppair.second;            
            if(!first) response << ", \n";            
            response << "{\n";
            response << "\"public_key\": \"" + p.public_key() + "\",";
            response << "\"entry_points\": [";
            bool sfirst = true;
            
            for(auto &e: p.entry_points())
            {
              if(!sfirst) response << ",\n";              
              response << "{";
              response << "\"shard\": " << e.shard  <<",";  
              response << "\"host\": \"" << e.host  <<"\",";
              response << "\"port\": " << e.port  << ",";
              response << "\"http_port\": " << e.http_port  << ",";
              response << "\"configuration\": " << e.configuration  << "}";              
              sfirst = false;              
            }
            
            response << "]";
            response << "}";
            first = false;            
          }
          
        });
      
      response << "]}";
      std::cout << response.str() << std::endl;
            
      return fetch::http::HTTPResponse(response.str());
    };            

    http_server_.AddView("/list/outgoing",  list_outgoing);


    auto list_incoming = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      std::stringstream response;
      
      response << "{\"incoming\": [";  
      
      swarm_->with_client_details_do([&response](std::map< uint64_t, NodeDetails > const &peers) {
          bool first = true;          
          for(auto &ppair: peers)
          {
            auto &p = ppair.second;            
            if(!first) response << ", \n";            
            response << "{\n";
            response << "\"public_key\": \"" + p.public_key() + "\",";
            response << "\"entry_points\": [";
            bool sfirst = true;
            
            for(auto &e: p.entry_points())
            {
              if(!sfirst) response << ",\n";              
              response << "{";
              response << "\"shard\": " << e.shard  <<",";  
              response << "\"host\": \"" << e.host  <<"\",";
              response << "\"port\": " << e.port  << ",";
              response << "\"http_port\": " << e.http_port  << ",";
              response << "\"configuration\": " << e.configuration  << "}";              
              sfirst = false;              
            }
            
            response << "]";
            response << "}";
            first = false;            
          }
          
        });
      
      response << "]}";
      std::cout << response.str() << std::endl;
            
      return fetch::http::HTTPResponse(response.str());
    };            

    http_server_.AddView("/list/incoming",  list_incoming);
    

    
    auto list_suggestions = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      std::stringstream response;
      
      response << "{\"suggestions\": [";  
      
      swarm_->with_suggestions_do([&response](std::vector< NodeDetails > const &peers) {
          bool first = true;          
          for(auto &p: peers)
          {
            if(!first) response << ", \n";            
            response << "{\n";
            response << "\"public_key\": \"" + p.public_key() + "\",";
            response << "\"entry_points\": [";
            bool sfirst = true;
            
            for(auto &e: p.entry_points())
            {
              if(!sfirst) response << ",\n";              
              response << "{";
              response << "\"shard\": " << e.shard  <<",";  
              response << "\"host\": \"" << e.host  <<"\",";
              response << "\"port\": " << e.port  << ",";
              response << "\"http_port\": " << e.http_port  << ",";
              response << "\"configuration\": " << e.configuration  << "}";              
              sfirst = false;              
            }
            
            response << "]";
            response << "}";
            first = false;            
          }
          
        });
      
      response << "]}";
      std::cout << response.str() << std::endl;
            
      return fetch::http::HTTPResponse(response.str());
    };        
    http_server_.AddView("/list/suggestions",  list_suggestions);
    

    
    auto node_details = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      std::string response = "{\"name\": \"hello world\"";
      
      response += "}";      
      return fetch::http::HTTPResponse(response);
    };    
    http_server_.AddView("/node-details",  node_details);         
    
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
