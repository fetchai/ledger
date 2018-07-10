#ifndef STORAGE_OBJECT_STORE_PROTOCOL_HPP
#define STORAGE_OBJECT_STORE_PROTOCOL_HPP
#include "storage/object_store.hpp"
#include "network/service/protocol.hpp"
namespace fetch {
namespace storage {

template< typename T >
class ObjectStoreProtocol : public fetch::service::Protocol {
public:
  using self_type = ObjectStoreProtocol<T>;
  
  enum {
    GET = 0,
    SET = 1
  };
  
  ObjectStoreProtocol(ObjectStore<T> *obj_store) : fetch::service::Protocol() {
    obj_store_ = obj_store;    
    this->Expose(GET, this, &self_type::Get);
    this->Expose(SET, obj_store, &ObjectStore<T>::Set);
  }


private:
  T Get(ResourceID const &rid) 
  {
    T ret;
    obj_store_->Get(rid, ret);
    return ret;    
  }
  
  ObjectStore<T> *obj_store_;
  
};

}
}

#endif
