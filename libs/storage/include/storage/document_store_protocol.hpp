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
  
  RevertibleDocumentStoreProtocol(RevertibleDocumentStore *doc_store) : fetch::service::Protocol() {
    this->Expose(GET, doc_store, &RevertibleDocumentStore::Get);
    this->Expose(SET, doc_store, &RevertibleDocumentStore::Set);
    this->Expose(COMMIT, doc_store, &RevertibleDocumentStore::Commit);
    this->Expose(REVERT, doc_store, &RevertibleDocumentStore::Revert);
    this->Expose(HASH, doc_store, &RevertibleDocumentStore::Hash);
  }
};


}
}

#endif
