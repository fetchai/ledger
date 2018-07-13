#include<iostream>
#include"network/service/client.hpp"
#include"core/logger.hpp"
#include"core/commandline/cli_header.hpp"
#include"storage/document_store_protocol.hpp"
#include"core/byte_array/tokenizer/tokenizer.hpp"
#include"core/commandline/parameter_parser.hpp"
#include"core/byte_array/consumers.hpp"
#include"core/json/document.hpp"
#include"ledger/storage_unit/lane_remote_control.hpp"
#include"ledger/storage_unit/storage_unit_client.hpp"
#include"core/string/trim.hpp"
#include"ledger/chain/helper_functions.hpp"
#include"core/byte_array/decoders.hpp"
using namespace fetch;

using namespace fetch::service;
using namespace fetch::byte_array;


  
int main(int argc, char const **argv) {
  typedef ServiceClient service_type;
  typedef fetch::network::TCPClient  client_type;

  typedef std::shared_ptr< service_type > shared_service_type;
  
  fetch::logger.DisableLogger();
  commandline::ParamsParser params;
  params.Parse(argc, argv);
  uint32_t lane_count =  params.GetParam<uint32_t>("lane-count", 1);  
  
  std::cout << std::endl;
  fetch::commandline::DisplayCLIHeader("Storage Unit Remote");
  std::cout << "Connecting with " << lane_count << " lanes." << std::endl;
    
  
  // Remote setup
  fetch::network::NetworkManager tm(8);
  std::string host = "localhost";
  uint16_t port = 8080;
  
  ledger::LaneRemoteControl remote;
  std::vector< shared_service_type > services;
  
  for(uint32_t i=0; i < lane_count; ++i) {
    client_type client( tm ) ;
    client.Connect(host, uint16_t(port +i));

    shared_service_type service = std::make_shared< service_type >(client, tm);
        
    services.push_back(service);
    
    remote.AddClient(i, service);
  }


  // Client setup
  fetch::ledger::StorageUnitClient client(tm);    

  tm.Start();

  for(std::size_t i = 0 ; i < lane_count; ++i) {
    client.AddLaneConnection< fetch::network::TCPClient >("localhost", uint16_t(8080 + i)) ;
  }

  
  // Setting tokenizer up
  enum {
    TOKEN_NAME = 1,
    TOKEN_STRING = 2,
    TOKEN_NUMBER = 3,
    TOKEN_CATCH_ALL = 12
  };
  std::string line = "";
  Tokenizer tokenizer;
  std::vector< ByteArray > command;
  
  tokenizer.AddConsumer( consumers::StringConsumer< TOKEN_STRING > );
  tokenizer.AddConsumer( consumers::NumberConsumer< TOKEN_NUMBER> );
  tokenizer.AddConsumer( consumers::Token< TOKEN_NAME> );
  tokenizer.AddConsumer( consumers::AnyChar< TOKEN_CATCH_ALL> );    
  
  while( (std::cin) && (line !="quit") ) {
    std::cout << ">> ";    

    // Getting command
    std::getline(std::cin, line);
    try
    {
      
      command.clear();
      tokenizer.clear();
      tokenizer.Parse( line );

      for(auto &t: tokenizer ) {
        if(t.type() != TOKEN_CATCH_ALL) command.push_back(t);
      }

    
      if(command.size() > 0) {
        if(command[0] == "connect") {
          if(command.size() == 4) {
            remote.Connect(uint32_t(command[1].AsInt()), command[2], uint16_t(command[3].AsInt()));
          } else {
            std::cout << "usage: connect [lane] [ip] [port]" << std::endl;
          }
        }

        if(command[0] == "getlanenumber") {
          if(command.size() == 2) {
            std::cout << remote.GetLaneNumber(uint32_t(command[1].AsInt())) << std::endl;
          } else {
            std::cout << "usage: getlanenumber [lane]" << std::endl;
          }
        }        

        if(command[0] == "gettx") {
          if(command.size() != 2) {
            std::cout << "usage: gettx \"[hash]\"" << std::endl;
          } else {
            auto enckey = command[1].SubArray(1, command[1].size() -2);
            auto key = byte_array::FromBase64(enckey);
            
            chain::Transaction tx;
            client.GetTransaction( key, tx );
            std::cout << std::endl;
            
            std::cout << "Transaction: " << byte_array::ToBase64(tx.digest()) << std::endl;
            std::cout << "Signature: " << byte_array::ToBase64(tx.signature()) << std::endl;    
            std::cout << "Fee: " << tx.summary().fee << std::endl;
            std::cout << std::endl;              
          }
          
          
        }
        else if(command[0] == "addtx") {
          if(command.size() == 1) {
            chain::Transaction tx = chain::VerifiedTransaction::Create( chain::RandomTransaction());
            std::cout << std::endl;            
            std::cout << "Transaction: " << byte_array::ToBase64(tx.digest()) << std::endl;
            std::cout << "Signature: " << byte_array::ToBase64(tx.signature()) << std::endl;    
            std::cout << "Fee: " << tx.summary().fee << std::endl;
            std::cout << std::endl;
            
            client.AddTransaction( tx );
            
          } else {
            std::cout << "usage: addtx" << std::endl;
          }
        }        

      }
      
    } catch(serializers::SerializableException &e ) {
        std::cerr << "error: " << e.what() << std::endl;
        
        
      }
        
        
  }
  

  tm.Stop();
  
  return 0;
  
}
