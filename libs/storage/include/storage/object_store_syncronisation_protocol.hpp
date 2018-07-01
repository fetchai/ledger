#ifndef STORAGE_OBJECT_STORE_SYNCRONISATION_PROTOCOL_HPP
#define STORAGE_OBJECT_STORE_SYNCRONISATION_PROTOCOL_HPP
#include "storage/object_store.hpp"
#include "network/service/protocol.hpp"
namespace fetch {
namespace storage {
  
template< typename T >
class ObjectStoreSyncronisationProtocol : public fetch::service::Protocol {
public:
  enum {
    OBJECT_COUNT = 1,
    PULL_OBJECTS = 2,
    PULL_OLDER_OBJECTS = 3
  };
  typedef ObjectStoreSyncronisationProtocol< T > self_type;
  
  ObjectStoreSyncronisationProtocol(ObjectStore<T> *store) :
    fetch::service::Protocol(),
    store_(store) {

    this->Expose(OBJECT_COUNT, this, &self_type::ObjectCount);
    this->Expose(PULL_OBJECTS, this, &self_type::PullObjects);
    this->Expose(PULL_OLDER_OBJECTS, this, &self_type::PullOlderObjects);
  }

private:

  uint64_t ObjectCount() 
  {
    return 0;
  }

  std::vector< T > PullObjects() 
  {
    return std::vector< T > ();
  }

  std::vector< T > PullOlderObjects() 
  {
    return std::vector< T > ();
  }

  ObjectStore<T> *store_;  
};


}
}

#endif
