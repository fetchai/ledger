#ifndef STORAGE_OBJECT_STORE_PROTOCOL_HPP
#define STORAGE_OBJECT_STORE_PROTOCOL_HPP
#include "storage/object_store.hpp"
#include "network/service/protocol.hpp"
#include<functional>

namespace fetch {
namespace storage {

template< typename T >
class ObjectStoreProtocol : public fetch::service::Protocol {
public:
  using event_function_type = std::function< void(T const&) > ;
  
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

  void OnSetObject(event_function_type const &f) 
  {
    on_set_ = f;
    
  }
  

private:
  T Get(ResourceID const &rid) 
  {
    T ret;
    obj_store_->Get(rid, ret);
    return ret;    
  }
  
  ObjectStore<T> *obj_store_;
  event_function_type on_set_;
  
};

}
}

#endif
