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
  
  ShardProtocol(network::ThreadManager *thread_manager, uint64_t const &protocol) :
    ShardManager(),
    fetch::service::Protocol()
  {
    using namespace fetch::service;

    // RPC Protocol

//    auto hello = new CallableClassMember<ShardProtocol, NodeDetails(uint64_t, NodeDetails)>(Callable::CLIENT_ID_ARG, this, &ShardProtocol::Hello);

    auto ping = new CallableClassMember<ShardProtocol, uint64_t()>(this, &ShardProtocol::Ping);    
    auto push_transaction = new CallableClassMember<ShardProtocol, bool(transaction_type) >(this, &ShardProtocol::PushTransaction );
    auto push_block = new CallableClassMember<ShardProtocol, void(block_type) >(this, &ShardProtocol::PushBlock );
    auto get_block = new CallableClassMember<ShardProtocol, block_type() >(this, &ShardProtocol::GetNextBlock );
    auto commit = new CallableClassMember<ShardProtocol, void() >(this, &ShardProtocol::Commit );    
    
    Protocol::Expose(ShardRPC::PING, ping);   
    Protocol::Expose(ShardRPC::PUSH_TRANSACTION, push_transaction);
    Protocol::Expose(ShardRPC::PUSH_BLOCK, push_block);
    Protocol::Expose(ShardRPC::GET_NEXT_BLOCK, get_block);
    Protocol::Expose(ShardRPC::COMMIT, commit);    
    
    // Using the event feed that
    Protocol::RegisterFeed(ShardFeed::FEED_BROADCAST_TRANSACTION, this);
    
    // Web interface
    auto submit_transaction = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      
      return fetch::http::HTTPResponse("hello world");
    };
    
    HTTPModule::Get("/submit-transaction", submit_transaction); 
  }


  uint64_t Ping() 
  {
    return 1337;
  }


};

};
};


#endif
