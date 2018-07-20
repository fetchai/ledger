#include<iostream>
#include"network/service/client.hpp"
#include"core/logger.hpp"
#include"core/commandline/cli_header.hpp"
#include"storage/document_store_protocol.hpp"
#include"core/byte_array/tokenizer/tokenizer.hpp"
#include"core/commandline/parameter_parser.hpp"
#include"core/byte_array/consumers.hpp"
#include"core/json/document.hpp"
#include"ledger/chain/main_chain_remote_control.hpp"

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
  
  std::cout << std::endl;
  fetch::commandline::DisplayCLIHeader("Main Chain Remote");
  
  // Remote setup
  fetch::network::NetworkManager tm(8);
  std::string host = "localhost";
  uint16_t port =  params.GetParam<uint16_t>("port", 8080);  
  
  chain::MainChainRemoteControl remote;
  std::vector< shared_service_type > services;
  

  client_type client( tm ) ;
  client.Connect(host, port);
  shared_service_type service = std::make_shared< service_type >(client, tm);
  services.push_back(service);
  remote.SetClient(service);


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
          if(command.size() == 3) {
            remote.Connect(command[1], uint16_t(command[2].AsInt()));
          } else {
            std::cout << "usage: connect [mainchain] [ip] [port]" << std::endl;
          }
        }

        // TODO: Add block
      }
      
    } catch(serializers::SerializableException &e ) {
        std::cerr << "error: " << e.what() << std::endl;
        
        
      }
        
        
  }
  

  tm.Stop();
  
  return 0;
  
}
