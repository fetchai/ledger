#ifndef PROTOCOLS_CHAIN_KEEPER_PROTOCOL_HPP
#define PROTOCOLS_CHAIN_KEEPER_PROTOCOL_HPP
#include "service/function.hpp"
#include "protocols/chain_keeper/commands.hpp"
#include "protocols/chain_keeper/controller.hpp"
#include "http/module.hpp"
#include "byte_array/decoders.hpp"
#include "crypto/fnv.hpp"

#include<unordered_set>

namespace fetch
{
namespace protocols 
{

class ChainKeeperProtocol : public ChainKeeperController,
                      public fetch::service::Protocol,
                      public fetch::http::HTTPModule { 
public:
  typedef typename ChainKeeperController::transaction_summary_type transaction_summary_type;  
  typedef typename ChainKeeperController::transaction_type transaction_type;

  typedef fetch::service::ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type >  client_shared_ptr_type;

  
  ChainKeeperProtocol(network::ThreadManager *thread_manager,
    uint64_t const &protocol,
    EntryPoint& details) :
    ChainKeeperController(protocol, thread_manager, details),
    fetch::service::Protocol()
  {
    using namespace fetch::service;
    
    // RPC Protocol
    auto ping = new CallableClassMember<ChainKeeperProtocol, uint64_t()>(this, &ChainKeeperProtocol::Ping);
    auto hello = new CallableClassMember<ChainKeeperProtocol, EntryPoint(std::string)>(this, &ChainKeeperProtocol::Hello);        
    auto push_transaction = new CallableClassMember<ChainKeeperProtocol, bool(transaction_type) >(this, &ChainKeeperProtocol::PushTransaction );
    auto get_transactions = new CallableClassMember<ChainKeeperProtocol, std::vector< transaction_type >() >(this, &ChainKeeperProtocol::GetTransactions );
    auto get_summaries = new CallableClassMember<ChainKeeperProtocol, std::vector< transaction_summary_type >() >(this, &ChainKeeperProtocol::GetSummaries );
    
    
    Protocol::Expose(ChainKeeperRPC::PING, ping);
    Protocol::Expose(ChainKeeperRPC::HELLO, hello);    
    Protocol::Expose(ChainKeeperRPC::PUSH_TRANSACTION, push_transaction);
    Protocol::Expose(ChainKeeperRPC::GET_TRANSACTIONS, get_transactions);
    Protocol::Expose(ChainKeeperRPC::GET_SUMMARIES, get_summaries);        

    // TODO: Move to separate protocol
    auto listen_to = new CallableClassMember<ChainKeeperProtocol, void(std::vector< EntryPoint >) >(this, &ChainKeeperProtocol::ListenTo );
    auto set_group_number = new CallableClassMember<ChainKeeperProtocol, void(uint32_t, uint32_t) >(this, &ChainKeeperProtocol::SetGroupNumber );
    auto group_number = new CallableClassMember<ChainKeeperProtocol, uint32_t() >(this, &ChainKeeperProtocol::group_number );
    auto count_outgoing = new CallableClassMember<ChainKeeperProtocol,  uint32_t() >(this, &ChainKeeperProtocol::count_outgoing_connections );        
    
    Protocol::Expose(ChainKeeperRPC::LISTEN_TO,listen_to);
    Protocol::Expose(ChainKeeperRPC::SET_GROUP_NUMBER, set_group_number);
    Protocol::Expose(ChainKeeperRPC::GROUP_NUMBER, group_number);
    Protocol::Expose(ChainKeeperRPC::COUNT_OUTGOING_CONNECTIONS, count_outgoing);
    
    
    // Web interface
    auto connect_to = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {      
      this->ConnectTo( params["ip"] , params["port"].AsInt() );
      return fetch::http::HTTPResponse("{\"status\":\"ok\"}");
    };
    
    HTTPModule::Get("/group-connect-to/(ip=\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/(port=\\d+)", connect_to);


    auto all_details = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      LOG_STACK_TRACE_POINT;
      std::stringstream response;
      response << "{\"outgoing\": [";  
      this->with_peers_do([&response](std::vector< client_shared_ptr_type > const &, std::vector< EntryPoint > const&details) {
          bool first = true;          
          for(auto &d: details)
          {

            if(!first) response << ", \n";            
            response << "{\n";

            response << "\"group\": " << d.group  <<",";  
            response << "\"host\": \"" << d.host  <<"\",";
            response << "\"port\": " << d.port  << ",";
            response << "\"http_port\": " << d.http_port  << ",";
            response << "\"configuration\": " << d.configuration ;            

            response << "}";
            first = false;            
          }
          
        });
      
      response << "],";
      response << "\"transactions\": [";  
      this->with_transactions_do([&response](std::vector< transaction_type > const &alltxs) {
          bool first = true;
          std::size_t i = 0;
          
          for(auto const &t: alltxs)
          {
            auto sum = t.summary();            

            if(!first) response << ", \n";            
            response << "{\n";

            bool bfi = true;            
            response << "\"groups\": [";
            for(auto &g : sum.groups)  {
              if(!bfi) response << ", ";
              response << g;              
              bfi = false;
            }
            response << "],";
            response << "\"transaction_number\": " << i << ",";            
            response << "\"transaction_hash\": \"" << byte_array::ToBase64(sum.transaction_hash)  << "\"";
            response << "}";
            first = false;
            ++i;
            
          }
        });
      
      response << "]";
            
      response << "}";      
      std::cout << response.str() << std::endl;
            
      return fetch::http::HTTPResponse(response.str());      
    };
    HTTPModule::Get("/all-details",  all_details);

    
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

            response << "\"group\": " << d.group  <<",";  
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



    auto list_transactions = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      LOG_STACK_TRACE_POINT;
      std::stringstream response;
      response << "{\"transactions\": [";  
      this->with_transactions_do([&response](std::vector< transaction_type > const &alltxs) {
          bool first = true;
          std::size_t i = 0;
          
          for(auto const &t: alltxs)
          {
            auto sum = t.summary();            

            if(!first) response << ", \n";            
            response << "{\n";

            bool bfi = true;            
            response << "\"groups\": [";
            for(auto &g : sum.groups)  {
              if(!bfi) response << ", ";
              response << g;              
              bfi = false;
            }
            response << "],";
            response << "\"transaction_number\": " << i << ",";            
            response << "\"transaction_hash\": \"" << byte_array::ToBase64(sum.transaction_hash)  << "\"";
            response << "}";
            first = false;
            ++i;
            
          }
        });
      
      response << "]}";
      
      std::cout << response.str() << std::endl;
            
      return fetch::http::HTTPResponse(response.str());      
    };
    HTTPModule::Get("/list/transactions",  list_transactions);


    
    
    auto submit_transaction = [this, thread_manager](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      LOG_STACK_TRACE_POINT;
      thread_manager->Post([this, req]() {      
          json::JSONDocument doc = req.JSON();
          
          typedef fetch::chain::Transaction transaction_type;
          transaction_type tx;
          auto &res =  doc["resources"];

          
          for(std::size_t i=0; i < res.size(); ++i) {

            auto s = res[i].as_byte_array();
            byte_array::ByteArray group = byte_array::FromHex( s.SubArray(2, s.size() -2 ) );

            tx.PushGroup( group );

            
            
          }          
          tx.set_arguments( req.body() );
          this->PushTransaction( tx );
        });
      
            
      return fetch::http::HTTPResponse("{\"status\": \"ok\"}");
    };
        
    HTTPModule::Post("/group/submit-transaction", submit_transaction);

    
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
