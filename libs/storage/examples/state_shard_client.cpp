#include<iostream>
#include"network/service/client.hpp"
#include"core/logger.hpp"
#include"core/commandline/cli_header.hpp"
#include"storage/document_store_protocol.hpp"
#include"core/byte_array/tokenizer/tokenizer.hpp"
#include"core/byte_array/consumers.hpp"

using namespace fetch::service;
using namespace fetch::byte_array;


class SingleShardStateDBClient : private ServiceClient< fetch::network::TCPClient > 
{
public:
  SingleShardStateDBClient (std::string const &host, uint16_t const &port, fetch::network::ThreadManager &tm)
    : ServiceClient( host, port, tm)
  {
    id_ = "my-fetch-id";
  }
  
  ByteArray Get(ByteArray const &key) 
  {
    auto promise = this->Call(0, fetch::storage::DocumentStoreProtocol::GET, fetch::storage::ResourceID(key) );
    return promise.As<ByteArray>();
  }

  void Set(ByteArray const &key, ByteArray const &value) 
  {
    auto promise = this->Call(0, fetch::storage::DocumentStoreProtocol::SET, fetch::storage::ResourceID(key), value );
    promise.Wait(2000);
  }
  
  void Commit(uint64_t const &bookmark) 
  {
    auto promise = this->Call(0, fetch::storage::DocumentStoreProtocol::COMMIT, bookmark);
    promise.Wait(2000);
  }
  
  void Revert(uint64_t const &bookmark) 
  {
    auto promise = this->Call(0, fetch::storage::DocumentStoreProtocol::REVERT, bookmark);
    promise.Wait(2000);
  }  

  ByteArray Hash() 
  {
    return this->Call(0, fetch::storage::DocumentStoreProtocol::HASH).As<ByteArray>();
  }

  void SetID(ByteArray const&id) 
  {
    id_ = id;
  }
  
  ByteArray const &id() {
    return id_;
  }
private:
  ByteArray id_;
  
};


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
  SingleShardStateDBClient client("localhost", 8080, tm);

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
          std::cout << client.Get(command[1]) << std::endl;
        } else {
            std::cout << "usage: get [id]" << std::endl;
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
  }
  

  tm.Stop();
  
  return 0;
  
}
