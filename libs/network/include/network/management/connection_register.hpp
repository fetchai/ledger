#ifndef NETWORK_CONNECTION_MANAGER_HPP
#define NETWORK_CONNECTION_MANAGER_HPP
#include "network/management/abstract_connection_register.hpp"
#include "core/mutex.hpp"

#include<unordered_map>
#include<memory>

namespace fetch {
namespace network {


template< typename G >
class ConnectionRegisterImpl final : public AbstractConnectionRegister
{
public:
  
  typedef typename AbstractConnection::connection_handle_type connection_handle_type;
  typedef std::weak_ptr< AbstractConnection > weak_connection_type;
  typedef G details_type;
  
  struct  LockableDetails final : public details_type, public mutex::Mutex {  };

  ConnectionRegisterImpl() = default;
  ConnectionRegisterImpl(ConnectionRegisterImpl const &other) = delete;
  ConnectionRegisterImpl(ConnectionRegisterImpl &&other) = default;  
  ConnectionRegisterImpl& operator=(ConnectionRegisterImpl const &other) = delete;  
  ConnectionRegisterImpl& operator=(ConnectionRegisterImpl &&other) = default;
  
  virtual ~ConnectionRegisterImpl() = default;
      
  template< typename T, typename... Args >
  std::shared_ptr< T > CreateClient(Args &&...args) 
  {
    std::shared_ptr< T > connection = std::make_shared< T > (std::forward<Args>( args )... );
    auto wptr = connection->network_client_pointer();
    
    auto ptr = wptr.lock();
    assert( ptr );

    Enter(wptr);
    ptr->SetConnectionManager( shared_from_this() );

    return connection;
  }

  std::size_t size() const 
  {
    std::lock_guard< mutex::Mutex > lock( connections_lock_ );
    return  connections_.size();
  }

  void Leave(connection_handle_type const &id) override 
  {
    std::lock_guard< mutex::Mutex > lock( connections_lock_ );
    auto it =connections_.find( id );
    if( it != connections_.end() ) {
      connections_.erase(it);
    }
    
    auto it2 = details_.find( id );    
    if( it2 != details_.end() ) {
      details_.erase(it2);
    }    

  }

  void Enter(weak_connection_type  const &wptr) override
  {
    auto ptr = wptr.lock();
    if(ptr) {
      std::lock_guard< mutex::Mutex > lock( connections_lock_ );      
      connections_[ ptr->handle() ] = ptr;
      details_[ ptr->handle() ] = std::make_shared<LockableDetails>();
    }
  }
  
  std::shared_ptr< LockableDetails > GetDetails(connection_handle_type const &i) 
  {
    std::lock_guard< mutex::Mutex > lock( details_lock_ );
    return details_[i];
  }
  
private:
  mutable mutex::Mutex connections_lock_;
  std::unordered_map< connection_handle_type, weak_connection_type > connections_;
  mutable mutex::Mutex details_lock_;  
  std::unordered_map< connection_handle_type, std::shared_ptr< LockableDetails > >  details_;
  
};


template< typename G >
class ConnectionRegister 
{
public:
  typedef typename AbstractConnection::connection_handle_type connection_handle_type;
  typedef std::weak_ptr< AbstractConnection > weak_connection_type;
  typedef std::shared_ptr< ConnectionRegisterImpl<G> > shared_implementation_pointer_type;
  typedef typename ConnectionRegisterImpl<G>::LockableDetails lockable_details_type;
  
  ConnectionRegister () 
  {
    ptr_ = std::make_shared< ConnectionRegisterImpl<G> >();
  }
    
  template< typename T, typename... Args >
  std::shared_ptr< T > CreateClient(Args &&...args) 
  {
    return ptr_->template CreateClient< T, Args... >( std::forward<Args>( args )...  );
  }

  std::size_t size() const 
  {
    return ptr_->size();
  }

  void Enter(weak_connection_type wptr) 
  {
    ptr_->Enter(wptr);
  }

  std::shared_ptr< lockable_details_type > GetDetails(connection_handle_type const &i) 
  {
    return ptr_->GetDetails(i);
  }
  

  shared_implementation_pointer_type pointer() { return ptr_; }  
private:
  shared_implementation_pointer_type ptr_;
   
};

  

}
}
#endif
