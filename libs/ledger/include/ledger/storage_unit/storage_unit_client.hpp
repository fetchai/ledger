#ifndef LEDGER_STORAGE_UNIT_STORAGE_UNIT_CLIENT_HPP
#define LEDGER_STORAGE_UNIT_STORAGE_UNIT_CLIENT_HPP
#include"core/logger.hpp"
#include"network/service/client.hpp"
#include"network/tcp/connection_register.hpp"
#include"network/service/client.hpp"
#include"ledger/storage_unit/lane_identity.hpp"
#include"ledger/storage_unit/lane_identity_protocol.hpp"
#include"ledger/storage_unit/lane_service.hpp"
#include"ledger/storage_unit/lane_connectivity_details.hpp"

#include"storage/document_store_protocol.hpp"
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
  
  typedef service::ServiceClient service_client_type;
  typedef std::shared_ptr< service_client_type > shared_service_client_type;
  using client_register_type = fetch::network::ConnectionRegister< ClientDetails >;
  using connection_handle_type = client_register_type::connection_handle_type;
  using network_manager_type = fetch::network::ThreadManager;
  using lane_type = LaneIdentity::lane_type;
  
  StorageUnitClient( network_manager_type tm):
    network_manager_(tm)
  {    
    id_ = "my-fetch-id";
    // libs/ledger/include/ledger/chain/helper_functions.hpp 

  }

  void SetNumberOfLanes(lane_type const &count ) 
  {
    lanes_.resize(count);
    SetLaneLog2(uint32_t(lanes_.size()));
    assert( count == (1 << log2_lanes_) );
  }
  

  template< typename T >
  void AddLaneConnection(byte_array::ByteArray const&host, uint16_t const &port ) 
  {
    shared_service_client_type client = register_.template CreateServiceClient<T >( network_manager_, host, port);

    // Waiting for connection to be open
    std::size_t n = 0;    
    while( n < 10 ){
      auto p = client->Call(LaneService::IDENTITY,  LaneIdentityProtocol::PING);
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
      return;      
    }
    
          
    // Exchaning info
    auto p1 = client->Call(LaneService::IDENTITY, LaneIdentityProtocol::GET_LANE_NUMBER);
    auto p2 = client->Call(LaneService::IDENTITY, LaneIdentityProtocol::GET_TOTAL_LANES);    
    p1.Wait(1000);
    p2.Wait(1000);    

    lane_type lane = p1.As<lane_type>();
    lane_type total_lanes = p2.As<lane_type>();
    if(total_lanes > lanes_.size()) {
      lanes_.resize(total_lanes);
      SetLaneLog2(uint32_t(lanes_.size()));
      assert( lanes_.size() == (1 << log2_lanes_) );      
    }
    
    assert(lane < lanes_.size());
    fetch::logger.Debug("Adding lane ", lane);
    
    lanes_[lane] = client;
  }
  
  
  byte_array::ByteArray Get(byte_array::ByteArray const &key) 
  {
    auto res = fetch::storage::ResourceAddress(key) ;
    std::size_t lane = res.lane( log2_lanes_ );    

    auto promise = lanes_[ lane ]->Call(LaneService::STATE, fetch::storage::RevertibleDocumentStoreProtocol::GET, res );
     
    return promise.As<storage::Document>().document;
  }

  bool Lock(byte_array::ByteArray const &key) 
  {
//    std::cout << "Locking: " << key << std::endl;
      
    auto res = fetch::storage::ResourceAddress(key) ;
    std::size_t lane = res.lane(  log2_lanes_ );    
    auto promise = lanes_[ lane ]->Call(LaneService::STATE, fetch::storage::RevertibleDocumentStoreProtocol::LOCK, res );
     
    return promise.As<bool>();
  }

  bool Unlock(byte_array::ByteArray const &key) 
  {
//    std::cout << "Unlocking: " << key << std::endl;
    
    auto res = fetch::storage::ResourceAddress(key) ;
    std::size_t lane = res.lane(  log2_lanes_  );    
    auto promise = lanes_[ lane ]->Call(LaneService::STATE, fetch::storage::RevertibleDocumentStoreProtocol::UNLOCK, res );
     
    return promise.As<bool>();
  }
  
    
  void Set(byte_array::ByteArray const &key, byte_array::ByteArray const &value) 
  {
    auto res = fetch::storage::ResourceAddress(key) ;
    std::size_t lane = res.lane(  log2_lanes_  );
//    std::cout << "Setting " << key <<  " on lane " << lane << " " << byte_array::ToBase64(res.id()) << std::endl;    
    auto promise = lanes_[ lane ]->Call(LaneService::STATE, fetch::storage::RevertibleDocumentStoreProtocol::SET, res, value );
    promise.Wait(2000);
  }
  
  void Commit(uint64_t const &bookmark) 
  {
    std::vector< service::Promise > promises;    
    for(std::size_t i=0; i < lanes_.size(); ++i) {      
      auto promise = lanes_[i]->Call(LaneService::STATE, fetch::storage::RevertibleDocumentStoreProtocol::COMMIT, bookmark);
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
      auto promise = lanes_[i]->Call(LaneService::STATE, fetch::storage::RevertibleDocumentStoreProtocol::REVERT, bookmark);
      promises.push_back(promise);
    }

    for(auto &p: promises) {
      p.Wait();
    }
  }  

  byte_array::ByteArray Hash() 
  {
    //TODO
    return lanes_[0]->Call(LaneService::STATE, fetch::storage::RevertibleDocumentStoreProtocol::HASH).As<byte_array::ByteArray>();
  }

  void SetID(byte_array::ByteArray const&id) 
  {
    id_ = id;
  }

  void AddTransaction(chain::Transaction &tx) 
  {
//    tx.UpdateDigests();
    
  }

  
  byte_array::ByteArray const &id() {
    return id_;
  }


private:
  network_manager_type network_manager_;
  uint32_t log2_lanes_ = 0;

  void SetLaneLog2(lane_type const &count) 
  {
    log2_lanes_ = uint32_t((sizeof(uint32_t) << 3) - uint32_t(__builtin_clz(uint32_t(count)) + 1));
  }
  
  client_register_type register_;
  byte_array::ByteArray id_;
  std::vector< shared_service_client_type > lanes_;
  
};

}
}

#endif
