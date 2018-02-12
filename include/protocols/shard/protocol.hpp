#ifndef PROTOCOLS_SHARD_PROTOCOL_HPP
#define PROTOCOLS_SHARD_PROTOCOL_HPP
#include "service/function.hpp"
#include "protocols/shard/commands.hpp"
#include "protocols/shard/manager.hpp"
#include "http/module.hpp"


namespace fetch
{
namespace protocols 
{

class ShardProtocol : public ShardManager,
                      public fetch::service::Protocol,
                      public fetch::http::HTTPModule { 
public:
  typedef typename ShardManager::transaction_type transaction_type;
  typedef typename ShardManager::block_type block_type;  

  typedef fetch::service::ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type >  client_shared_ptr_type;

  
  ShardProtocol(network::ThreadManager *thread_manager,
    uint64_t const &protocol,
    EntryPoint& details) :
    ShardManager(protocol, thread_manager, details),
    fetch::service::Protocol()
  {
    using namespace fetch::service;

    
    // RPC Protocol
    auto ping = new CallableClassMember<ShardProtocol, uint64_t()>(this, &ShardProtocol::Ping);
    auto hello = new CallableClassMember<ShardProtocol, EntryPoint(std::string)>(this, &ShardProtocol::Hello);        
    auto push_transaction = new CallableClassMember<ShardProtocol, bool(transaction_type) >(this, &ShardProtocol::PushTransaction );
    auto push_block = new CallableClassMember<ShardProtocol, void(block_type) >(this, &ShardProtocol::PushBlock );
    auto get_block = new CallableClassMember<ShardProtocol, block_type() >(this, &ShardProtocol::GetNextBlock );
    auto commit = new CallableClassMember<ShardProtocol, void() >(this, &ShardProtocol::Commit );    
    
    Protocol::Expose(ShardRPC::PING, ping);
    Protocol::Expose(ShardRPC::HELLO, hello);    
    Protocol::Expose(ShardRPC::PUSH_TRANSACTION, push_transaction);
    Protocol::Expose(ShardRPC::PUSH_BLOCK, push_block);
    Protocol::Expose(ShardRPC::GET_NEXT_BLOCK, get_block);
    Protocol::Expose(ShardRPC::COMMIT, commit);    
    
    // Using the event feed that
    Protocol::RegisterFeed(ShardFeed::FEED_BROADCAST_TRANSACTION, this);
    
    // Web interface
    auto connect_to = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {      
      this->ConnectTo( params["ip"] , params["port"].AsInt() );
      return fetch::http::HTTPResponse("{\"status\":\"ok\"}");
    };
    
    HTTPModule::Get("/connect-to/(ip=\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/(port=\\d+)", connect_to);

    
    auto list_outgoing = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      std::stringstream response;
      response << "{\"outgoing\": [";  
      this->with_peers_do([&response](std::vector< client_shared_ptr_type > const &, std::vector< EntryPoint > const&details) {
          bool first = true;          
          for(auto &d: details)
          {

            if(!first) response << ", \n";            
            response << "{\n";

            response << "\"shard\": " << d.shard  <<",";  
            response << "\"host\": \"" << d.host  <<"\",";
            response << "\"port\": " << d.port  << ",";
            response << "\"http_port\": " << d.http_port  << ",";
            response << "\"configuration\": " << d.configuration ;            

            response << "}";
            first = false;            
          }
          
        });
      
      response << "]}";
      
      std::cout << response.str() << std::endl;
            
      return fetch::http::HTTPResponse(response.str());      
    };
    HTTPModule::Get("/list/outgoing",  list_outgoing);
    
    
    auto submit_transaction = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {      
      return fetch::http::HTTPResponse("hello world");
    };
    
    HTTPModule::Post("/shard/submit-transaction", submit_transaction);

    auto next_block = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      
      return fetch::http::HTTPResponse("hello world");
    };
    
    HTTPModule::Get("/shard/next-block", next_block);

    auto submit_block = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      
      return fetch::http::HTTPResponse("hello world");
    };    
    HTTPModule::Post("/shard/submit-block", submit_block);
    
  }


  uint64_t Ping() 
  {
    return 1337;
  }


};

};
};


#endif
