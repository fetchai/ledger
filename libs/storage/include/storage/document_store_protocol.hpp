#ifndef STORAGE_INDEXED_DOCUMENT_STORE_PROTOCOL_HPP
#define STORAGE_INDEXED_DOCUMENT_STORE_PROTOCOL_HPP
#include "storage/document_store.hpp"
#include "network/service/protocol.hpp"
namespace fetch {
namespace storage {

class DocumentStoreProtocol : public fetch::service::Protocol {
public:
  enum {
    GET = 0,
    SET = 1,
    COMMIT = 5,
    REVERT = 6,
    HASH = 7
  };
  
  DocumentStoreProtocol(DocumentStore *doc_store) : fetch::service::Protocol() {
    this->Expose(GET, doc_store, &DocumentStore::Get);
    this->Expose(SET, doc_store, &DocumentStore::Set);
    this->Expose(COMMIT, doc_store, &DocumentStore::Commit);
    this->Expose(REVERT, doc_store, &DocumentStore::Revert);
    this->Expose(HASH, doc_store, &DocumentStore::Hash);
  }
};


}
}

#endif
