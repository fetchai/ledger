#ifndef STORAGE_OBJECT_STORE_PROTOCOL_HPP
#define STORAGE_OBJECT_STORE_PROTOCOL_HPP
#include "storage/object_store.hpp"
#include "network/service/protocol.hpp"
namespace fetch {
namespace storage {

template< typename T >
class ObjectStoreProtocol : public fetch::service::Protocol {
public:
  enum {
    GET = 0,
    SET = 1
  };
  
  ObjectStoreProtocol(ObjectStore<T> *obj_store) : fetch::service::Protocol() {
    this->Expose(GET, obj_store, &ObjectStore<T>::Get);
    this->Expose(SET, obj_store, &ObjectStore<T>::Set);
  }
};

}
}

#endif
