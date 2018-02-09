#include<iostream>
#include<functional>

#include"commandline/parameter_parser.hpp"

#include "service/client.hpp"
#include"service/server.hpp"
#include"network/tcp_server.hpp"
#include"protocols.hpp"


#include"http/server.hpp"
#include"http/middleware/allow_origin.hpp"
#include"http/middleware/color_log.hpp"

#include"crypto/hash.hpp"
#include"unittest.hpp"
#include"byte_array/encoders.hpp"
#include"random/lfg.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>


using namespace fetch::protocols;
using namespace fetch::commandline;
using namespace fetch::byte_array;

typedef typename ShardManager::transaction_type tx_type;


class FetchShardService {
public:
  FetchShardService(uint16_t port) :   
    thread_manager_( new fetch::network::ThreadManager(8) ),
    service_(port, thread_manager_)
  {
    
    std::cout << "Listening for peers on " << (port) << ", clients on " << ( port ) << std::endl;
      
    // Creating a service contiaing the shard protocol
    shard_ =  new ShardProtocol(thread_manager_, FetchProtocols::SHARD);
    service_.Add(FetchProtocols::SHARD, shard_ );


    // Creating a http server based on the shard protocol
//    http_server_.AddMiddleware( fetch::http::middleware::AllowOrigin("*") );       
//    http_server_.AddMiddleware( fetch::http::middleware::ColorLog);
//    http_server_.AddModule(*shard_);
    
  }
  
  ~FetchShardService() 
  {
    std::cout << "Killing fetch service";    
    delete shard_;
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
//  fetch::http::HTTPServer http_server_;  
  
  ShardProtocol *shard_ = nullptr;
};



FetchShardService *service;
void StartService(int port) 
{
  service = new FetchShardService(port);
  service->Start();
}


int main(int argc, char const **argv) {
  ParamsParser params;
  params.Parse(argc, argv);
 
  if(params.arg_size() < 2) 
  {
    std::cout << "usage: " << argv[0] << " [port]" << std::endl;
    exit(-1);
  }

  uint16_t  my_port = params.GetArg<uint16_t>(1);  
  StartService( my_port );

  std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );  


  std::cout << "Press Ctrl+C to stop" << std::endl;  
  while(true) {
    std::this_thread::sleep_for( std::chrono::milliseconds(200) );   

  }

  
  service->Stop();
  delete service;
  

  
  return 0;
}
