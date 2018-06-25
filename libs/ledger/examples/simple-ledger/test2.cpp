#define FETCH_DISABLE_TODO_COUT
#include<iostream>
#include<functional>

#include"commandline/parameter_parser.hpp"

#include "service/client.hpp"
#include"service/server.hpp"
#include"network/tcp_server.hpp"
#include"protocols/shard.hpp"


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


enum FetchProtocols 
{
   SHARD = 2
};


std::vector< std::string > words = {"squeak", "fork", "governor", "peace", "courageous", "support", "tight", "reject", "extra-small", "slimy", "form", "bushes", "telling", "outrageous", "cure", "occur", "plausible", "scent", "kick", "melted", "perform", "rhetorical", "good", "selfish", "dime", "tree", "prevent", "camera", "paltry", "allow", "follow", "balance", "wave", "curved", "woman", "rampant", "eatable", "faulty", "sordid", "tooth", "bitter", "library", "spiders", "mysterious", "stop", "talk", "watch", "muddle", "windy", "meal", "arm", "hammer", "purple", "company", "political", "territory", "open", "attract", "admire", "undress", "accidental", "happy", "lock", "delicious"}; 

fetch::random::LaggedFibonacciGenerator<> lfg;
tx_type RandomTX(std::size_t const &n) {

  std::string ret = words[ lfg() & 63 ];
  for(std::size_t i=1; i < n;++i) {
    ret += " " + words[lfg() & 63];
  }
  tx_type t;
  t.set_body(ret);
  return t;
}

class FetchShardService {
public:
  FetchShardService(uint16_t port) :   
    thread_manager_( new fetch::network::ThreadManager(8) ),
    service_(port, thread_manager_),
    http_server_(8080, thread_manager_)
  {
    
    std::cout << "Listening for peers on " << (port) << ", clients on " << ( port ) << std::endl;
      
    // Creating a service contiaing the shard protocol
    shard_ =  new ShardProtocol(thread_manager_, FetchProtocols::SHARD);
    service_.Add(FetchProtocols::SHARD, shard_ );


    // Creating a http server based on the shard protocol
    http_server_.AddMiddleware( fetch::http::middleware::AllowOrigin("*") );       
    http_server_.AddMiddleware( fetch::http::middleware::ColorLog);
    http_server_.AddModule(*shard_);
    
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
  fetch::http::HTTPServer http_server_;  
  
  ShardProtocol *shard_ = nullptr;
};



FetchShardService *service;
void StartService(int port) 
{
  service = new FetchShardService(port);
  service->Start();
}


int main(int argc, char const **argv) {
  StartService( 1337 );

  std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );  

  typedef fetch::service::ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type >  client_shared_ptr_type;
  
  fetch::network::ThreadManager tm(2);    
  client_shared_ptr_type client = std::make_shared< client_type >("localhost", 1337, &tm);
  tm.Start();
  
  std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );    

  auto ping_promise = client->Call(FetchProtocols::SHARD, ShardRPC::PING);
  if(!ping_promise.Wait( 2000 )) 
  {
    fetch::logger.Error("Client not repsonding - hanging up!");
    exit(-1);    
  }         
  

  ShardManager manager;

  for(std::size_t i = 0; i < 100; ++i) {
    
    auto tx = RandomTX(4) ;  
    std::cout << tx.body() << std::endl;
    

    client->Call(FetchProtocols::SHARD, ShardRPC::PUSH_TRANSACTION, tx );
    manager.PushTransaction( tx );

    auto server_block = client->Call(FetchProtocols::SHARD, ShardRPC::GET_NEXT_BLOCK ).As<ShardManager::block_type >(); 
    auto local_block = manager.GetNextBlock();
    

    if( (server_block.body().previous_hash != local_block.body().previous_hash) ||
      (server_block.header() != local_block.header() ) ||
      (server_block.body().transaction_hash != local_block.body().transaction_hash) ) {
      std::cout << "FAILED" << std::endl;
  
      std::cout << "Server block: " << server_block.meta_data().block_number << " " <<  server_block.meta_data().total_work <<  std::endl;
      std::cout << "  <- " << fetch::byte_array::ToBase64( server_block.body().previous_hash ) << std::endl;
      std::cout << "   = " << fetch::byte_array::ToBase64( server_block.header() ) << std::endl;
      std::cout << "    (" << fetch::byte_array::ToBase64( server_block.body().transaction_hash ) << ")" << std::endl;  
      std::cout << "Local block: " << local_block.meta_data().block_number << " " <<  local_block.meta_data().total_work <<  std::endl;
      std::cout << "  <- " << fetch::byte_array::ToBase64( local_block.body().previous_hash ) << std::endl;
      std::cout << "   = " << fetch::byte_array::ToBase64( local_block.header() ) << std::endl;
      std::cout << "    (" << fetch::byte_array::ToBase64( local_block.body().transaction_hash ) << ")" << std::endl;
      exit(-1);      
    }

    auto &p1 = server_block.proof();    
    p1.SetTarget( lfg() % 5 );
    while(!p1()) ++p1;

    auto &p2 = local_block.proof();    
    p2.SetTarget( lfg() % 5 );
    while(!p2()) ++p2;

    client->Call(FetchProtocols::SHARD, ShardRPC::PUSH_BLOCK, server_block );
    client->Call(FetchProtocols::SHARD, ShardRPC::PUSH_BLOCK, local_block );    

    manager.PushBlock(local_block);
    manager.PushBlock(server_block);    
    
    client->Call(FetchProtocols::SHARD, ShardRPC::COMMIT);          
    manager.Commit();
  }
  
  
  tm.Stop();
  
  service->Stop();
  delete service;
  

  
  return 0;
}
