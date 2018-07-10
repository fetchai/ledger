#ifndef LEDGER_STORAGE_UNIT__HPP
#define LEDGER_STORAGE_UNIT_LANE_CONTROLLER_HPP
#include"network/service/client.hpp"
#include"core/logger.hpp"
#include"core/commandline/cli_header.hpp"
#include"storage/document_store_protocol.hpp"
#include"core/byte_array/tokenizer/tokenizer.hpp"
#include"core/commandline/parameter_parser.hpp"
#include"core/byte_array/consumers.hpp"
#include"core/json/document.hpp"
#include"ledger/chain/transaction.hpp"


namespace fetch
{
namespace ledger
{

class StorageUnitClient
{
public:
  struct ClientDetails 
  {
    std::atomic< uint32_t > lane;
  };
  
  typedef ServiceClient service_client_type;
  typedef std::shared_ptr< service_client_type > shared_service_client_type;
  using client_register_type = fetch::network::ConnectionRegister< ClientDetails >;
  using connection_handle_type = client_register_type::connection_handle_type;
  using lane_type = LaneIdentity::lane_type;
  
  StorageUnitClient( fetch::network::ThreadManager &tm):
    thread_manager_(tm)
  {    
    id_ = "my-fetch-id";
    // libs/ledger/include/ledger/chain/helper_functions.hpp 

  }

  void SetNumberOfLanes(lane_type const &count ) 
  {
    lanes.resize(count);
    SetLaneLog2(lanes.size());
    assert( count == (1 << lane_log2_) );
  }
  

  template< typename T >
  void AddLaneConnection(byte_array::ByteArray const&host, uint16_t const &port ) 
  {
    shared_service_client_type client = register_.CreateServiceClient<T >( thread_manager_, host, port);

    // Waiting for connection to be open
    std::size_t n = 0;    
    while( n < 10 ){
      auto p = client->Call(identity_protocol_, LaneIdentityProtocol::PING);
      if(p.Wait(100, false)) {
        if(p.As<LaneIdentity::ping_type >() != LaneIdentity::PING_MAGIC) {
          n = 10;          
        }
        break;        
      }
      ++n;
    }
    if(n >= 10) {
      logger.Warn("Connection timed out - closing");
      client->Close();
      client.reset();
      return nullptr;      
    }
    
          
    // Exchaning info
    auto p = client->Call(LaneService::IDENTITY, LaneIdentityProtocol::GET_LANE_NUMBER);
    p.Wait(1000);

    lane_type lane = p.As<lane_type>();
    asserte(lane < lanes.size());
  
    lanes[lane] = client;
  }
  
  
  ByteArray Get(ByteArray const &key) 
  {
    auto res = fetch::storage::ResourceAddress(key) ;
    std::size_t lane = res.lane( lane_log2_ );    

    auto promise = lanes_[ lane ]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::GET, res );
     
    return promise.As<storage::Document>().document;
  }

  bool Lock(ByteArray const &key) 
  {
//    std::cout << "Locking: " << key << std::endl;
      
    auto res = fetch::storage::ResourceAddress(key) ;
    std::size_t lane = res.lane(  lane_log2_ );    
    auto promise = lanes_[ lane ]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::LOCK, res );
     
    return promise.As<bool>();
  }

  bool Unlock(ByteArray const &key) 
  {
//    std::cout << "Unlocking: " << key << std::endl;
    
    auto res = fetch::storage::ResourceAddress(key) ;
    std::size_t lane = res.lane(  lane_log2_  );    
    auto promise = lanes_[ lane ]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::UNLOCK, res );
     
    return promise.As<bool>();
  }
  
    
  void Set(ByteArray const &key, ByteArray const &value) 
  {
    auto res = fetch::storage::ResourceAddress(key) ;
    std::size_t lane = res.lane(  lane_log2_  );
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


  void AddTransaction(ConstByteArray const& tx_data)
  {
    json::JSONDocument doc(tx_data);
    chain::Transaction tx;
    
  }

  void AddTransaction(chain::Transaction &tx) 
  {
//    tx.UpdateDigests();
    
  }

  
  ByteArray const &id() {
    return id_;
  }


private:
  uint32_t log2_lanes_;

  void SetLaneLog2(lane_type const &count) 
  {
    log2_lanes_ = (sizeof(uint32_t) << 3) - __builtin_clz(uint32_t(count)) - 1;
  }
  
  
  ByteArray id_;
  std::vector< shared_service_client_type > lanes_;
  
};

}
}

#endif
