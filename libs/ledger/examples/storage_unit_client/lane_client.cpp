#include<iostream>
#include"network/service/client.hpp"
#include"core/logger.hpp"
#include"core/commandline/cli_header.hpp"
#include"storage/document_store_protocol.hpp"
#include"core/byte_array/tokenizer/tokenizer.hpp"
#include"core/commandline/parameter_parser.hpp"
#include"core/byte_array/consumers.hpp"
#include"core/json/document.hpp"
#include"ledger/chain/transaction.hpp"
#include"ledger/storage_unit/storage_unit_client.hpp"
#include"core/string/trim.hpp"

using namespace fetch;

using namespace fetch::ledger;
using namespace fetch::byte_array;


enum {
  TOKEN_NAME = 1,
  TOKEN_STRING = 2,
  TOKEN_NUMBER = 3,
  TOKEN_CATCH_ALL = 12
};


  
int main(int argc, char const **argv) {
  // Parameters
  fetch::logger.DisableLogger();
  commandline::ParamsParser params;
  params.Parse(argc, argv);
  uint32_t lane_count =  params.GetParam<uint32_t>("lane-count", 1);  
  
  std::cout << std::endl;
  fetch::commandline::DisplayCLIHeader("Storage Unit Client");
  std::cout << "Connecting with " << lane_count << " lanes." << std::endl;
    
  
  // Client setup
  fetch::network::ThreadManager tm(8);
  StorageUnitClient client(tm);

  tm.Start();

  for(std::size_t i = 0 ; i < lane_count; ++i) {
    client.AddLaneConnection< fetch::network::TCPClient >("localhost", uint16_t(8080 + i)) ;
  }
  


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
        if(command[0] == "addtx") {
          if(command.size() == 1) {
            std::cout << "TODO: Implement" << std::endl;
          } else {
            std::cout << "usage: add" << std::endl;
          }
        } else if(command[0] == "get") {
          if(command.size() == 2) {
            std::cout << client.Get(command[1]) << std::endl;
          } else {
            std::cout << "usage: get [id]" << std::endl;
          }
        } else if(command[0] == "lock") {
        
          if(command.size() == 2) {
            client.Lock(command[1]);
          } else {
              std::cout << "usage: lock [id]" << std::endl;
            }
        } else if(command[0] == "unlock") {
            if(command.size() == 2) {
              client.Unlock(command[1]);
            } else {
              std::cout << "usage: unlock [id]" << std::endl;
            }
            
          } else if(command[0] == "set") {
            if(command.size() == 3) {
              client.Set(command[1], command[2]);
            } else {
              std::cout << "usage: set [id] \"[value]\"" << std::endl;
            }
        
          } else if(command[0] == "commit") {
            if(command.size() == 2) {
              uint64_t bookmark = uint64_t(command[1].AsInt());
              client.Commit( bookmark );
            } else {
              std::cout << "usage: commit [bookmark,int]" << std::endl;
            }
          } else if(command[0] == "revert") {
            if(command.size() == 2) {
              uint64_t bookmark = uint64_t(command[1].AsInt());         
              client.Revert( bookmark );
            } else {
              std::cout << "usage: revert [bookmark,int]" << std::endl;
            }
          } else if(command[0] == "hash") {
            if(command.size() == 1) {
              ByteArray barr = client.Hash();
              std::cout << "State hash: " << ToBase64( barr ) << std::endl;
         
            } else {
              std::cout << "usage: set [id] \"[value]\"" << std::endl;
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
