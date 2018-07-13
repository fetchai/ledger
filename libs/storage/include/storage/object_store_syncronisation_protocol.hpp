#ifndef STORAGE_OBJECT_STORE_SYNCRONISATION_PROTOCOL_HPP
#define STORAGE_OBJECT_STORE_SYNCRONISATION_PROTOCOL_HPP
#include "storage/object_store.hpp"
#include"network/details/thread_pool.hpp"
#include"network/service/promise.hpp"
#include "network/service/protocol.hpp"
#include"core/logger.hpp"

#include<vector>
#include<deque>
namespace fetch {
namespace storage {
  
template<typename R, typename T, typename S = T >
class ObjectStoreSyncronisationProtocol : public fetch::service::Protocol {
public:
  enum {
    OBJECT_COUNT = 1,
    PULL_OBJECTS = 2,
    PULL_OLDER_OBJECTS = 3
  };
  using self_type = ObjectStoreSyncronisationProtocol< R, T, S >;
  using protocol_handler_type = service::protocol_handler_type;
  using register_type = R;
  using thread_pool_type = network::ThreadPool;
  
  ObjectStoreSyncronisationProtocol(protocol_handler_type const &p, register_type const& r, thread_pool_type const&nm, ObjectStore<T> *store) :
    fetch::service::Protocol(),
    protocol_(p),
    register_(r),
    thread_pool_(nm),
    store_(store),
    running_(false) { // , register_(reg), manager_(nm) 

    logger.Info("Exposing ", p, ":", PULL_OBJECTS);
    
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


  void Start() 
  {
    fetch::logger.Debug("Starting syncronisation of ", typeid(T).name());    
    if(running_) return;    
    running_ = true;
    thread_pool_->Post([this]() { this->IdleUntilPeers(); } );
  }

  void Stop() 
  {
    running_ = false;
  }
  
  
  
  /// Methods for pull transactions from peers
  /// @{

  void IdleUntilPeers() 
  {
    if(!running_) return;
    
    if(register_.number_of_services() == 0 ) {
      thread_pool_->Post([this]() { this->IdleUntilPeers(); }, 1000 ); // TODO: Make time variable
    } else {          
      thread_pool_->Post([this]() { this->FetchObjectsFromPeers(); } );
    }
  }
  

  
  void FetchObjectsFromPeers() 
  {
    fetch::logger.Debug("Fetching objects ", typeid(T).name(), " from peer");
    
    if(!running_) return;
    
    std::lock_guard< mutex::Mutex > lock( object_list_mutex_);
    
    using service_map_type = typename R::service_map_type;
    register_.WithServices([this](service_map_type const &map) {
        for(auto const &p: map) {
          if(!running_) return;
          
          auto peer = p.second;
          auto ptr = peer.lock();
          uint64_t from = 0; // TODO: Get variable
          object_list_promises_.push_back( ptr->Call(protocol_, PULL_OBJECTS, from) );

        }
      });

    if(running_) {
      thread_pool_->Post([this]() { this->RealisePromises(); } );
    }
    
  }
  

  void RealisePromises() 
  {
    if(!running_) return;    
    std::lock_guard< mutex::Mutex > lock( object_list_mutex_);
    incoming_objects_.reserve(uint64_t(max_cache_));
    
    new_objects_.clear();
      
    for(auto &p : object_list_promises_) {
      
      if(!running_) return;
      
      incoming_objects_.clear();
      if(!p.Wait(100, false)) {
        continue;
      }
      
      p.template As< std::vector< S > >( incoming_objects_ );

      // TODO: Update pointer
      
      if(!running_) return;

      std::lock_guard< mutex::Mutex > lock(mutex_);

        
      store_->WithLock([this]() {
          
          for(auto &o: incoming_objects_ ) {
            CachedObject obj;
            obj.data = T::Create(o);        
            ResourceID rid(obj.data.digest());
            
            if(store_->LocklessHas( rid  )) continue;        
            store_->LocklessSet(  rid, obj.data );
            cache_.push_back( obj );        
            
          }
          
        });

      
    }

    object_list_promises_.clear();
    if(running_) {
      thread_pool_->Post([this]() { this->IdleUntilPeers(); }, 100 ); // TODO: Make time parameter      
    }
    
  }
  /// @}


  void AddToCache(T const & o) 
  {
    std::lock_guard< mutex::Mutex > lock(mutex_);
    CachedObject obj;
    obj.data = o;
    cache_.push_back(obj);
    
  }
  
private:
  protocol_handler_type protocol_;
  register_type register_;
  thread_pool_type thread_pool_;
   
  uint64_t ObjectCount() 
  {
    std::lock_guard< mutex::Mutex > lock(mutex_);    
    return cache_.size() + uint64_t(first_);
  }

  std::vector< S > PullObjects(uint64_t const &from)
  {
    std::lock_guard< mutex::Mutex > lock(mutex_);

    if(cache_.begin() == cache_.end()) {
      return std::vector< S > ();
    }
    
    uint64_t first = from - first_;    
    if(from < first_ ) {
      TODO("Cannot currently handle back-log");
      first = 0;
    }
    
    if(first >= cache_.size()) return std::vector< S > ();
                                
    uint64_t N = cache_.size() - first;
    
    // Creating result
    std::vector<S> ret;    
    
    for(uint64_t i=first; i < N; ++i) {
      ++cache_[i].passed_on;
      ret.push_back(  cache_[i].data );
    }
    
    return ret;

  }

  void PushObjects(std::vector< S > const& objs)
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

  
  uint64_t first_ = 0;
  uint64_t max_cache_ = 2000;  


  mutable mutex::Mutex object_list_mutex_;
  std::vector< service::Promise > object_list_promises_;
  std::vector< T > new_objects_;
  std::vector< S > incoming_objects_;

  std::atomic< bool > running_ ;

  
};


}
}

#endif
