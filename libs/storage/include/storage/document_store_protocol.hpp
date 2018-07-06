#ifndef STORAGE_INDEXED_DOCUMENT_STORE_PROTOCOL_HPP
#define STORAGE_INDEXED_DOCUMENT_STORE_PROTOCOL_HPP
#include "storage/document_store.hpp"
#include "storage/revertible_document_store.hpp"
#include "network/service/protocol.hpp"
#include "core/mutex.hpp"
#include"core/mutex.hpp"


#include<map>
namespace fetch {
namespace storage {

  

class RevertibleDocumentStoreProtocol :
    public fetch::service::Protocol {
public:
  typedef network::AbstractConnection::connection_handle_type connection_handle_type;
  enum {
    GET = 0,
    LAZY_GET,
    SET,
    COMMIT,
    REVERT,
    HASH ,

    LOCK = 20,
    UNLOCK ,
    HAS_LOCK
  };
  
  RevertibleDocumentStoreProtocol(RevertibleDocumentStore *doc_store)
    : fetch::service::Protocol(), doc_store_(doc_store) {
    this->Expose(GET, (RevertibleDocumentStore::super_type*)doc_store, &RevertibleDocumentStore::super_type::Get);
    this->Expose(SET, (RevertibleDocumentStore::super_type*)doc_store, &RevertibleDocumentStore::super_type::Set);
    this->Expose(COMMIT, doc_store, &RevertibleDocumentStore::Commit);
    this->Expose(REVERT, doc_store, &RevertibleDocumentStore::Revert);
    this->Expose(HASH, doc_store, &RevertibleDocumentStore::Hash);

    this->ExposeWithClientArg(LOCK, this, &RevertibleDocumentStoreProtocol::LockResource);
    this->ExposeWithClientArg(UNLOCK, this, &RevertibleDocumentStoreProtocol::UnlockResource);
    this->ExposeWithClientArg(HAS_LOCK, this, &RevertibleDocumentStoreProtocol::HasLock);    
  }

  RevertibleDocumentStoreProtocol(RevertibleDocumentStore *doc_store, uint32_t const &lane, uint32_t const &maxlanes)
    : fetch::service::Protocol(), doc_store_(doc_store), max_lanes_(maxlanes), lane_assignment_(lane) {
    this->Expose(GET, this, &RevertibleDocumentStoreProtocol::GetLaneChecked);
    this->ExposeWithClientArg(SET, this, &RevertibleDocumentStoreProtocol::SetLaneChecked);

    this->Expose(COMMIT, doc_store, &RevertibleDocumentStore::Commit);
    this->Expose(REVERT, doc_store, &RevertibleDocumentStore::Revert);
    this->Expose(HASH, doc_store, &RevertibleDocumentStore::Hash);
    
    this->ExposeWithClientArg(LOCK, this, &RevertibleDocumentStoreProtocol::LockResource);
    this->ExposeWithClientArg(UNLOCK, this, &RevertibleDocumentStoreProtocol::UnlockResource);
    this->ExposeWithClientArg(HAS_LOCK, this, &RevertibleDocumentStoreProtocol::HasLock);
  }

  bool HasLock(connection_handle_type const &client_id, ResourceID const &rid) 
  {
    std::lock_guard< mutex::Mutex > lock( lock_mutex_ );
    auto it = locks_.find(rid.id());
    if(it == locks_.end()) return false;
    
    return (it->second == client_id);
  }
  
  bool LockResource(connection_handle_type const &client_id, ResourceID const &rid) 
  {
    std::lock_guard< mutex::Mutex > lock( lock_mutex_ );
    auto it = locks_.find(rid.id());
    if(it == locks_.end()) {
      locks_[rid.id()] = client_id;
      return true;
    }
    
    return (it->second == client_id);
  }

  bool UnlockResource(connection_handle_type const &client_id, ResourceID const &rid) 
  {

    std::lock_guard< mutex::Mutex > lock( lock_mutex_ );
    auto it = locks_.find(rid.id());
    if(it == locks_.end()) {
      return false;
    }
    
    if (it->second == client_id) {
      locks_.erase( it );      
      return true;      
    }

    return false;   
  }
  
  
private:
  Document GetLaneChecked(ResourceID const &rid) 
  {
    if(lane_assignment_ != rid.lane( max_lanes_ )) {
      throw serializers::SerializableException( // TODO: set exception number
        0, byte_array_type("Get: Resource located on other lane. TODO, set error number"));
    }
    
    return doc_store_->Get(rid);
  }

  void SetLaneChecked(connection_handle_type const &client_id, ResourceID const &rid, byte_array::ConstByteArray const& value) 
  {
    if(lane_assignment_ != rid.lane( max_lanes_ )) {
      throw serializers::SerializableException( // TODO: set exception number
        0, byte_array_type("Set: Resource located on other lane. TODO: Set error number."));
    }
    {
      std::lock_guard< mutex::Mutex > lock( lock_mutex_ );
      auto it = locks_.find(rid.id());
      if((it == locks_.end()) ||
        (it->second != client_id) ) {
        throw serializers::SerializableException( // TODO: set exception number
          0, byte_array_type("Client does not have a lock for the resource"));
      }      
    }

    doc_store_->Set(rid, value);
  }
  
  RevertibleDocumentStore *doc_store_;  
  uint32_t max_lanes_ = 0;  
  uint32_t lane_assignment_ = 0;

  mutex::Mutex lock_mutex_;
  std::map< byte_array::ConstByteArray, connection_handle_type > locks_;  
};


}
}

#endif
