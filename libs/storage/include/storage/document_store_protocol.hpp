#ifndef STORAGE_INDEXED_DOCUMENT_STORE_PROTOCOL_HPP
#define STORAGE_INDEXED_DOCUMENT_STORE_PROTOCOL_HPP
#include "storage/document_store.hpp"
#include "network/service/protocol.hpp"
namespace fetch {
namespace storage {

class RevertibleDocumentStoreProtocol : public fetch::service::Protocol {
public:
  enum {
    GET = 0,
    SET = 1,
    COMMIT = 5,
    REVERT = 6,
    HASH = 7
  };
  
  RevertibleDocumentStoreProtocol(RevertibleDocumentStore *doc_store)
    : fetch::service::Protocol(), doc_store_(doc_store) {
    this->Expose(GET, doc_store, &RevertibleDocumentStore::Get);
    this->Expose(SET, doc_store, &RevertibleDocumentStore::Set);
    this->Expose(COMMIT, doc_store, &RevertibleDocumentStore::Commit);
    this->Expose(REVERT, doc_store, &RevertibleDocumentStore::Revert);
    this->Expose(HASH, doc_store, &RevertibleDocumentStore::Hash);
  }

  RevertibleDocumentStoreProtocol(RevertibleDocumentStore *doc_store, uint32_t const &lane, uint32_t const &maxlanes)
    : fetch::service::Protocol(), doc_store_(doc_store), lane_assigment_(lane), max_lanes_(maxlanes) {
    this->Expose(GET, this, &RevertibleDocumentStoreProtocol::Get);
    this->Expose(SET, this, &RevertibleDocumentStoreProtocol::Set);

    this->Expose(COMMIT, doc_store, &RevertibleDocumentStore::Commit);
    this->Expose(REVERT, doc_store, &RevertibleDocumentStore::Revert);
    this->Expose(HASH, doc_store, &RevertibleDocumentStore::Hash);
  }
private:
  byte_array::ConstByteArray Get(ResourceID const &rid) 
  {
    if(lane_assignment_ != rid.lane( max_lanes_ )) {
      throw serializers::SerializableException( // TODO: set exception number
        0, byte_array_type("Get: Resource located on other lane. TODO, set error number"));
    }
    
    return doc_store_->Get(rid);
  }

  byte_array::ConstByteArray Set(ResourceID const &rid, byte_array::ConstByteArray const& value) 
  {
    if(lane_assignment_ != rid.lane( max_lanes_ )) {
      throw serializers::SerializableException( // TODO: set exception number
        0, byte_array_type("Set: Resource located on other lane. TODO: Set error number."));
    }
    
    return doc_store_->Set(rid, value);
  }
  
  RevertibleDocumentStore *doc_store_;  
  uint32_t max_lanes_ = 0;  
  uint32_t lane_assignment_ = 0;
};


}
}

#endif
