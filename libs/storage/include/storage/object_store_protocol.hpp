#pragma once
#include "storage/object_store.hpp"
#include "network/service/protocol.hpp"
#include<functional>

namespace fetch {
namespace storage {

template< typename T >
class ObjectStoreProtocol : public fetch::service::Protocol {
public:
  using event_set_object_type = std::function< void(T const&) > ;
  
  using self_type = ObjectStoreProtocol<T>;
  
  enum {
    GET = 0,
    SET,
    HAS
  };
  
  ObjectStoreProtocol(ObjectStore<T> *obj_store) : fetch::service::Protocol() {
    obj_store_ = obj_store;    
    this->Expose(GET, this, &self_type::Get);
    this->Expose(SET, this, &self_type::Set);
    this->Expose(HAS, obj_store, &ObjectStore<T>::Has);    
  }

  void OnSetObject(event_set_object_type const &f) 
  {
    on_set_ = f;
  }
  

private:
  void Set(ResourceID const &rid, T const &object) 
  {
    if(on_set_)
    {
      on_set_( object );      
    }
    
    obj_store_->Set(rid, object);
  }
  
    


  T Get(ResourceID const &rid) 
  {
    T ret;
    obj_store_->Get(rid, ret);
    return ret;    
  }
  
  ObjectStore<T> *obj_store_;
  event_set_object_type on_set_;
  
};

}
}

