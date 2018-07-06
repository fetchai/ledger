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
#include"core/string/trim.hpp"
using namespace fetch;

using namespace fetch::service;
using namespace fetch::byte_array;


  
int main(int argc, char const **argv) {
  typedef ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type > shared_client_type;
  
  fetch::logger.DisableLogger();
  commandline::ParamsParser params;
  params.Parse(argc, argv);
  uint32_t lane_count =  params.GetParam<uint32_t>("lane-count", 1);  
  
  std::cout << std::endl;
  fetch::commandline::DisplayCLIHeader("Multi-lane remote");
  std::cout << "Connecting with " << lane_count << " lanes." << std::endl;
    
  
  // Client setup
  fetch::network::ThreadManager tm(8);
  std::string host = "localhost";
  uint16_t port = 8080;
  
  ledger::LaneRemoteControl remote;
  std::vector< shared_client_type > clients;  
  for(uint32_t i=0; i < lane_count; ++i) {
    auto client = std::make_shared< client_type >(host, uint16_t(port +i), tm ) ;
    clients.push_back(client);
    remote.AddClient(i, client);
  }

  tm.Start();

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
          if(command.size() == 1) {
            remote.Connect(0, "localhnost", 8080);            
          } else {
            std::cout << "usage: connect" << std::endl;
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
