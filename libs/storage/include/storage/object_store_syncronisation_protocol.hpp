#ifndef STORAGE_OBJECT_STORE_SYNCRONISATION_PROTOCOL_HPP
#define STORAGE_OBJECT_STORE_SYNCRONISATION_PROTOCOL_HPP
#include "storage/object_store.hpp"
#include "network/service/protocol.hpp"

#include<vector>
#include<deque>
namespace fetch {
namespace storage {
  
template< typename T, typename S = T >
class ObjectStoreSyncronisationProtocol : public fetch::service::Protocol {
public:
  enum {
    OBJECT_COUNT = 1,
    PULL_OBJECTS = 2,
    PULL_OLDER_OBJECTS = 3
  };
  using self_type = ObjectStoreSyncronisationProtocol< T, S >;
  
  ObjectStoreSyncronisationProtocol(ObjectStore<T> *store) :
    fetch::service::Protocol(),
    store_(store){ // , register_(reg), manager_(nm) 

    this->Expose(OBJECT_COUNT, this, &self_type::ObjectCount);
    this->Expose(PULL_OBJECTS, this, &self_type::PullObjects);
//    this->Expose(PULL_OLDER_OBJECTS, this, &self_type::PullOlderObjects);
  }


  void PushObject(S const &obj) 
  {
    std::lock_guard< mutex::Mutex > lock(mutex_);    
    CachedObject c;
    c.data = T( obj );
    cache_.push_back( c );
  }

  template<typename R>
  void FetchObjectsFromPeers(R &peer_register) 
  {


  }
  

  
  void FlushCache() 
  {
    TODO_FAIL("Yet to be implemented");
  }
  
  
private:
  uint64_t ObjectCount() 
  {
    std::lock_guard< mutex::Mutex > lock(mutex_);    
    return cache_.size() + uint64_t(first_);
  }

  std::vector< S > PullObjects(int64_t const &from)
  {
    std::lock_guard< mutex::Mutex > lock(mutex_);
    return std::vector< T > ();    
/*    
    if(cache_.begin() == cache_.end()) {      
      return std::vector< T > ();
    }
    
    int64_t first = from - first_;
    
    if(first < 0 ) {
      TODO("Cannot currently handle back-log");
      
      first = 0;
    }
    
    if(first >= cache_.size()) return std::vector< T > ();
                                
    uint64_t N = int64_t(cache_.size()) - first;

    // Creating result
    std::vector<S> ret;    
    ret.resize(N);
    
    for(uint64_t i=first; i < N; ++i) {
      ++cache_[i].passed_on;
      ret.push_back( S( cache_[i].data ) );
    }
    
    return ret;
*/
  }

  void PushObjects(std::vector< S > const& txs)
  {
    std::lock_guard< mutex::Mutex > lock(mutex_);

    // TODO: Implement.
    
  }


  struct CachedObject 
  {
    T data;
    int passed_on = 0;
  };  
    
  
  mutex::Mutex mutex_;  
  ObjectStore<T> *store_;
  
  std::deque< CachedObject > cache_;


  int64_t first_ = 0;
  int64_t max_cache_ = 2000;  
  
};


}
}

#endif
