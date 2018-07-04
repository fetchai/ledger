#include<iostream>
#include"network/service/client.hpp"
#include"core/logger.hpp"
#include"core/commandline/cli_header.hpp"
#include"storage/document_store_protocol.hpp"
#include"core/byte_array/tokenizer/tokenizer.hpp"
#include"core/commandline/parameter_parser.hpp"
#include"core/byte_array/consumers.hpp"
using namespace fetch;

using namespace fetch::service;
using namespace fetch::byte_array;


class MultiLaneDBClient  //: private 
{
public:
  typedef ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type > shared_client_type;
  
  MultiLaneDBClient (uint32_t lanes, std::string const &host, uint16_t const &port, fetch::network::ThreadManager &tm)
  {
    id_ = "my-fetch-id";
    for(uint32_t i = 0; i < lanes; ++i) {
      lanes_.push_back( std::make_shared< client_type >(host, uint16_t(port +i), tm ) );
    }
    
  }
  
  ByteArray Get(ByteArray const &key) 
  {
    auto res = fetch::storage::ResourceAddress(key) ;
    std::size_t lane = res.lane( uint32_t(lanes_.size() ));    
//    std::cout << "Getting " << key << " from lane " << lane <<  " " << byte_array::ToBase64(res.id()) << std::endl;
    auto promise = lanes_[ lane ]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::GET, res );
     
    return promise.As<storage::Document>().document;
  }

  bool Lock(ByteArray const &key) 
  {
//    std::cout << "Locking: " << key << std::endl;
      
    auto res = fetch::storage::ResourceAddress(key) ;
    std::size_t lane = res.lane( uint32_t(lanes_.size() ));    
    auto promise = lanes_[ lane ]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::LOCK, res );
     
    return promise.As<bool>();
  }

  bool Unlock(ByteArray const &key) 
  {
//    std::cout << "Unlocking: " << key << std::endl;
    
    auto res = fetch::storage::ResourceAddress(key) ;
    std::size_t lane = res.lane( uint32_t(lanes_.size() ));    
    auto promise = lanes_[ lane ]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::UNLOCK, res );
     
    return promise.As<bool>();
  }
  
    
  void Set(ByteArray const &key, ByteArray const &value) 
  {
    auto res = fetch::storage::ResourceAddress(key) ;
    std::size_t lane = res.lane( uint32_t(lanes_.size() ));
//    std::cout << "Setting " << key <<  " on lane " << lane << " " << byte_array::ToBase64(res.id()) << std::endl;    
    auto promise = lanes_[ lane ]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::SET, res, value );
    promise.Wait(2000);
  }
  
  void Commit(uint64_t const &bookmark) 
  {
    std::vector< service::Promise > promises;    
    for(std::size_t i=0; i < lanes_.size(); ++i) {      
      auto promise = lanes_[i]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::COMMIT, bookmark);
      promises.push_back(promise);
    }

    for(auto &p: promises) {
      p.Wait();
    }
    
  }
  
  void Revert(uint64_t const &bookmark) 
  {
    std::vector< service::Promise > promises;    
    for(std::size_t i=0; i < lanes_.size(); ++i) {      
      auto promise = lanes_[i]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::REVERT, bookmark);
      promises.push_back(promise);
    }

    for(auto &p: promises) {
      p.Wait();
    }
  }  

  ByteArray Hash() 
  {
    //TODO
    return lanes_[0]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::HASH).As<ByteArray>();
  }

  void SetID(ByteArray const&id) 
  {
    id_ = id;
  }

/*
  void AddTransaction(ConstByteArray const& tx)
  {
    json::Document doc(tx);
    chain::Transaction tx;
    
  }

  void AddTransaction(chain::Transaction &tx) 
  {
    tx.UpdateDigests();
    
  }
*/  
  
  ByteArray const &id() {
    return id_;
  }


private:
  ByteArray id_;
  std::vector< shared_client_type > lanes_;
  
};


enum {
  TOKEN_NAME = 1,
  TOKEN_STRING = 2,
  TOKEN_NUMBER = 3,
  TOKEN_CATCH_ALL = 12
};

  
int main(int argc, char const **argv) {
  fetch::logger.DisableLogger();
  commandline::ParamsParser params;
  params.Parse(argc, argv);
  uint32_t lane_count =  params.GetParam<uint32_t>("lane-count", 1);  
  
  std::cout << std::endl;
  fetch::commandline::DisplayCLIHeader("Multi-lane client");
  
  // Client setup
  fetch::network::ThreadManager tm(8);
  MultiLaneDBClient client(lane_count, "localhost", 8080, tm);

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
    try
    {
      
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
