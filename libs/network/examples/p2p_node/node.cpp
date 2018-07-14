#include<iostream>
#include"network/service/server.hpp"
#include"core/commandline/cli_header.hpp"
#include"core/commandline/parameter_parser.hpp"
#include"network/p2pservice/p2p_service.hpp"
#include"core/byte_array/consumers.hpp"

#include<sstream>
using namespace fetch;
using namespace fetch::p2p;
using namespace fetch::byte_array;
enum {
  TOKEN_NAME = 1,
  TOKEN_STRING = 2,
  TOKEN_NUMBER = 3,
  TOKEN_CATCH_ALL = 12
};

int main(int argc, char const **argv) 
{  
  // Reading config
  commandline::ParamsParser params;
  params.Parse(argc, argv);
  uint16_t port =  params.GetParam<uint16_t>("port", 8080);

  int log = params.GetParam<int>("showlog", 0);
  if(log == 0) {
    fetch::logger.DisableLogger();
  }
    
  fetch::commandline::DisplayCLIHeader("P2P Service");

  // Setting up
  fetch::network::NetworkManager tm(8);
  P2PService service(port, tm);
  
  tm.Start();
  
  // Taking commands
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
          if(command.size() != 2) {
            std::cout << "usage: connect [host] [port]" << std::endl;
            continue;
          }
          service.Connect(command[1], uint16_t(command[2].AsInt()));
        }
      }
      
    } catch(serializers::SerializableException &e ) {
        std::cerr << "error: " << e.what() << std::endl;
    }        
  }
  


  tm.Stop();


  return 0;
}
