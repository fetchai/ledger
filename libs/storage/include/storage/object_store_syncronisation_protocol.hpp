#ifndef STORAGE_OBJECT_STORE_SYNCRONISATION_PROTOCOL_HPP
#define STORAGE_OBJECT_STORE_SYNCRONISATION_PROTOCOL_HPP
#include "storage/object_store.hpp"
#include"network/details/thread_pool.hpp"
#include"network/service/promise.hpp"
#include "network/service/protocol.hpp"
#include"core/logger.hpp"

#include<vector>
#include<set>
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

    this->Expose(OBJECT_COUNT, this, &self_type::ObjectCount);
    this->ExposeWithClientArg(PULL_OBJECTS, this, &self_type::PullObjects);
//    this->Expose(PULL_OLDER_OBJECTS, this, &self_type::PullOlderObjects);
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
          object_list_promises_.push_back( ptr->Call(protocol_, PULL_OBJECTS) );

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
      thread_pool_->Post([this]() { this->TrimCache(); } ); 
    }
  }
  
  void TrimCache() 
  {
    std::lock_guard< mutex::Mutex > lock(mutex_);
    std::size_t i = 0;
    for(auto &c: cache_) {
      c.UpdateLifetime();
    }
    std::sort(cache_.begin(), cache_.end());
    
    
    while( (i < cache_.size()) &&
      ( (cache_.size() > max_cache_ )  ||
        (cache_.back().lifetime > max_cache_life_time_ ) ) ) {
      auto back = cache_.back();
//      std::cout << "DELETEING Object with " << back.lifetime << " ms lifetime" << std::endl;
      cache_.pop_back();
    }

    
    if(running_) {
      // TODO: Make time parameter
      thread_pool_->Post([this]() { this->IdleUntilPeers(); }, 5000 ); 
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
    return cache_.size() ; // TODO: Return store size
  }

  std::vector< S > PullObjects(uint64_t const &client_handle)
  {
    
    std::lock_guard< mutex::Mutex > lock(mutex_);

    /*
    for(auto &c: cache_) {
      std::cout << "--- " << byte_array::ToBase64( c.data.digest()) << std::endl;
    }
    */
    
    if(cache_.begin() == cache_.end()) {
      return std::vector< S > ();
    }
    
    // Creating result
    std::vector<S> ret;    
    
    for(auto &c: cache_) {
      if(c.delivered_to.find(client_handle) == c.delivered_to.end()) {
        c.delivered_to.insert( client_handle );
        ret.push_back(  c.data );
      }
      
    }
//    std::cout << "Sending " << ret.size() << std::endl;
    
    return ret;

  }

  // TODO: (`HUT`) : This is not efficient since it will be copying this map around
  struct CachedObject 
  {
    CachedObject() 
    {
      created = std::chrono::system_clock::now();
    }

    void UpdateLifetime() 
    {
      std::chrono::system_clock::time_point end =
        std::chrono::system_clock::now();
      lifetime = double(std::chrono::duration_cast<std::chrono::milliseconds>(
          end - created)
        .count());
    }

    bool operator<(CachedObject const&other) const
    {
      return lifetime < other.lifetime;
    }
        
    T data;
    std::unordered_set< uint64_t > delivered_to;
    std::chrono::system_clock::time_point created;
    double lifetime = 0;
    
    
  };  
 
  mutex::Mutex mutex_;  
  ObjectStore<T> *store_;
  
  std::vector< CachedObject > cache_;


  uint64_t max_cache_ = 2000;  
  double  max_cache_life_time_ = 20000; // TODO: Make cache configurable
  
  mutable mutex::Mutex object_list_mutex_;
  std::vector< service::Promise > object_list_promises_;
  std::vector< T > new_objects_;
  std::vector< S > incoming_objects_;

  std::atomic< bool > running_ ;

  
};


}
}

#endif
