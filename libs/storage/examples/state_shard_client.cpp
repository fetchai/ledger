#include<iostream>
#include"network/service/client.hpp"
#include"core/logger.hpp"
#include"core/commandline/cli_header.hpp"
#include"storage/indexed_document_store.hpp"
#include"core/byte_array/tokenizer/tokenizer.hpp"
#include"core/byte_array/consumers.hpp"

using namespace fetch::service;
using namespace fetch::byte_array;

enum {
  TOKEN_NAME = 1,
  TOKEN_STRING = 2,
  TOKEN_NUMBER = 3,  

  TOKEN_CATCH_ALL = 12
};

  
int main() {
  std::cout << std::endl;
  fetch::commandline::DisplayCLIHeader("Single state shard client");
  fetch::logger.DisableLogger();
  
  // Client setup
  fetch::network::ThreadManager tm(2);
  ServiceClient< fetch::network::TCPClient > client("localhost", 8080, tm);

  tm.Start();

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
    command.clear();
    tokenizer.clear();
    tokenizer.Parse( line );

    for(auto &t: tokenizer ) {
      if(t.type() != TOKEN_CATCH_ALL) command.push_back(t);
    }

    
    if(command.size() > 0) {
      
      if(command[0] == "get") {
        if(command.size() == 2) {
          auto promise = client.Call(0, fetch::storage::DocumentStoreProtocol::GET, command[1] );
          std::string ret = promise.As<std::string>();
          std::cout << ret << std::endl;
        } else {
            std::cout << "usage: get [id]" << std::endl;
          }
          
            
      } else if(command[0] == "set") {
       if(command.size() == 3) {
         client.Call(0, fetch::storage::DocumentStoreProtocol::SET, command[1], command[2]).Wait();
        } else {
         std::cout << "usage: set [id] \"[value]\"" << std::endl;
        }
        
      } else if(command[0] == "commit") {
       if(command.size() == 2) {
         uint64_t bookmark = uint64_t(command[1].AsInt());
         client.Call(0, fetch::storage::DocumentStoreProtocol::COMMIT, bookmark ).Wait();
        } else {
         std::cout << "usage: commit [bookmark,int]" << std::endl;
        }
      } else if(command[0] == "revert") {
       if(command.size() == 2) {
         uint64_t bookmark = uint64_t(command[1].AsInt());         
         client.Call(0, fetch::storage::DocumentStoreProtocol::REVERT, bookmark ).Wait();
        } else {
         std::cout << "usage: revert [bookmark,int]" << std::endl;
        }
      } else if(command[0] == "hash") {
       if(command.size() == 1) {
         ByteArray barr = client.Call(0, fetch::storage::DocumentStoreProtocol::HASH).As<ByteArray>();
         std::cout << "State hash: " << ToBase64( barr ) << std::endl;
         
        } else {
         std::cout << "usage: set [id] \"[value]\"" << std::endl;
        }
      }              
        
    }
  }
  
//  client.WithDecorators(aes, ... ).Call( MYPROTO,SLOWFUNCTION, 4, 3 );

  tm.Stop();
  
  return 0;
  
}
