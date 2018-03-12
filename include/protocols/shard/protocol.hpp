#ifndef PROTOCOLS_SHARD_PROTOCOL_HPP
#define PROTOCOLS_SHARD_PROTOCOL_HPP
#include "service/function.hpp"
#include "protocols/shard/commands.hpp"
#include "protocols/shard/controller.hpp"
#include "http/module.hpp"


namespace fetch
{
namespace protocols 
{

class ShardProtocol : public ShardController,
                      public fetch::service::Protocol,
                      public fetch::http::HTTPModule { 
public:
  typedef typename ShardController::transaction_type transaction_type;
  typedef typename ShardController::block_type block_type;  

  typedef fetch::service::ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type >  client_shared_ptr_type;

  
  ShardProtocol(network::ThreadManager *thread_manager,
    uint64_t const &protocol,
    EntryPoint& details) :
    ShardController(protocol, thread_manager, details),
    fetch::service::Protocol()
  {
    using namespace fetch::service;
    
    // RPC Protocol
    auto ping = new CallableClassMember<ShardProtocol, uint64_t()>(this, &ShardProtocol::Ping);
    auto hello = new CallableClassMember<ShardProtocol, EntryPoint(std::string)>(this, &ShardProtocol::Hello);        
    auto push_transaction = new CallableClassMember<ShardProtocol, bool(transaction_type) >(this, &ShardProtocol::PushTransaction );
    auto push_block = new CallableClassMember<ShardProtocol, void(block_type) >(this, &ShardProtocol::PushBlock );
    auto get_block = new CallableClassMember<ShardProtocol, block_type() >(this, &ShardProtocol::GetNextBlock );


    auto exchange_heads = new CallableClassMember<ShardProtocol, block_type(block_type) >(this, &ShardProtocol::ExchangeHeads );
    auto request_blocks_from = new CallableClassMember<ShardProtocol, std::vector< block_type >(block_header_type, uint16_t) >(this, &ShardProtocol::RequestBlocksFrom );

    
    Protocol::Expose(ShardRPC::PING, ping);
    Protocol::Expose(ShardRPC::HELLO, hello);    
    Protocol::Expose(ShardRPC::PUSH_TRANSACTION, push_transaction);
    Protocol::Expose(ShardRPC::PUSH_BLOCK, push_block);
    Protocol::Expose(ShardRPC::GET_NEXT_BLOCK, get_block);


    Protocol::Expose(ShardRPC::EXCHANGE_HEADS, exchange_heads);
    Protocol::Expose(ShardRPC::REQUEST_BLOCKS_FROM, request_blocks_from);    

    // TODO: Move to separate protocol
    auto listen_to = new CallableClassMember<ShardProtocol, void(EntryPoint) >(this, &ShardProtocol::ListenTo );
    auto set_shard_number = new CallableClassMember<ShardProtocol, void(uint32_t, uint32_t) >(this, &ShardProtocol::SetShardNumber );
    auto shard_number = new CallableClassMember<ShardProtocol, uint32_t() >(this, &ShardProtocol::shard_number );
    auto count_outgoing = new CallableClassMember<ShardProtocol,  uint32_t() >(this, &ShardProtocol::count_outgoing_connections );        
    
    Protocol::Expose(ShardRPC::LISTEN_TO,listen_to);
    Protocol::Expose(ShardRPC::SET_SHARD_NUMBER, set_shard_number);
    Protocol::Expose(ShardRPC::SHARD_NUMBER, shard_number);
    Protocol::Expose(ShardRPC::COUNT_OUTGOING_CONNECTIONS, count_outgoing);
    
    
    // Using the event feed that
    Protocol::RegisterFeed(ShardFeed::FEED_BROADCAST_BLOCK, this);    
    Protocol::RegisterFeed(ShardFeed::FEED_BROADCAST_TRANSACTION, this);
    
    // Web interface
    auto connect_to = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {      
      this->ConnectTo( params["ip"] , params["port"].AsInt() );
      return fetch::http::HTTPResponse("{\"status\":\"ok\"}");
    };
    
    HTTPModule::Get("/shard-connect-to/(ip=\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/(port=\\d+)", connect_to);
    
    auto list_outgoing = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      LOG_STACK_TRACE_POINT;
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


    
    auto list_blocks = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      LOG_STACK_TRACE_POINT;
      std::stringstream response;
      response << "{\"blocks\": [";  
      this->with_blocks_do([&response](ShardController::block_type const & head, std::map< ShardController::block_header_type, ShardController::block_type > chain) {

          response << "{";
          response << "\"block_hash\": \"" << byte_array::ToBase64( head.header() ) << "\",";
          response << "\"previous_hash\": \"" << byte_array::ToBase64( head.body().previous_hash ) << "\",";
          response << "\"transaction_hash\": \"" << byte_array::ToBase64( head.body().transaction_hash ) << "\",";
          response << "\"block_number\": " <<  head.meta_data().block_number  << ",";
          response << "\"total_work\": " <<  head.meta_data().total_work;          
          response << "}";

          auto next_hash = head.body().previous_hash;
          std::size_t i=0;
          
          while( (i< 10) && (chain.find( next_hash ) !=chain.end() ) ) {
            auto const &block = chain[next_hash];
            ++i;
            response << ", {";
            response << "\"block_hash\": \"" << byte_array::ToBase64( block.header() ) << "\",";
            response << "\"previous_hash\": \"" << byte_array::ToBase64( block.body().previous_hash ) << "\",";
            response << "\"transaction_hash\": \"" << byte_array::ToBase64( block.body().transaction_hash ) << "\",";
            response << "\"block_number\": " <<  block.meta_data().block_number  << ",";
            response << "\"total_work\": " <<  block.meta_data().total_work;          
            response << "}";            
            next_hash =  block.body().previous_hash;
          }

        });
      
      response << "]}";
      
      std::cout << response.str() << std::endl;
            
      return fetch::http::HTTPResponse(response.str());      
    };
    HTTPModule::Get("/list/blocks",  list_blocks);
    
    auto submit_transaction = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      LOG_STACK_TRACE_POINT;
      
      json::JSONDocument doc = req.JSON();
      
      std::cout << "resources " << doc["resources"] << std::endl;

      typedef fetch::chain::Transaction transaction_type;
      transaction_type tx;
      tx.set_arguments( req.body() );
      this->PushTransaction( tx );

      return fetch::http::HTTPResponse("{\"status\": \"ok\"}");
    };
    
    HTTPModule::Post("/shard/submit-transaction", submit_transaction);

    
  }


  uint64_t Ping() 
  {
    LOG_STACK_TRACE_POINT;
    
    fetch::logger.Debug("Responding to Ping request");    
    return 1337;
  }


};

};
};


#endif
