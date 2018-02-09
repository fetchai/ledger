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
  ShardProtocol(network::ThreadManager *thread_manager, uint64_t const &protocol, NodeDetails &details) :
    ShardManager(protocol, thread_manager, details),
    fetch::service::Protocol()
  {
    using namespace fetch::service;

    // RPC Protocol
    auto ping = new CallableClassMember<ShardProtocol, uint64_t()>(this, &ShardProtocol::Ping);
    auto hello = new CallableClassMember<ShardProtocol, NodeDetails(uint64_t, NodeDetails)>(Callable::CLIENT_ID_ARG, this, &ShardProtocol::Hello);
    
    Protocol::Expose(ShardRPC::PING, ping);
    Protocol::Expose(ShardRPC::HELLO, hello);
    
    // Using the event feed that
    Protocol::RegisterFeed(ShardFeed::FEED_REQUEST_CONNECTIONS, this);
    
    // Web interface
    auto submit_transaction = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      
      return fetch::http::HTTPResponse("hello world");
    }
    HTTPModule::Get("/submit-transaction", submit_transaction); 
  }
  
};

};
};


#endif
