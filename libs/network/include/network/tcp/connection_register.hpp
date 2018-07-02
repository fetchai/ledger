#ifndef NETWORK_CONNECTION_MANAGER_HPP
#define NETWORK_CONNECTION_MANAGER_HPP
#include "network/tcp/abstract_connection_register.hpp"
#include "core/mutex.hpp"

#include<unordered_map>
#include<memory>

namespace fetch {
namespace network {


class ConnectionRegisterImpl : public AbstractConnectionRegister
{
public:
  typedef typename AbstractConnection::connection_handle_type connection_handle_type;
  typedef std::weak_ptr< AbstractConnection > weak_connection_type;
  
  template< typename T, typename... Args >
  std::shared_ptr< T > CreateClient(Args &&...args) 
  {
    std::shared_ptr< T > connection = std::make_shared< T > (std::forward<Args>( args )... );
    auto wptr = connection->network_client_pointer();
    
    auto ptr = wptr.lock();
    assert( ptr );

    {
      std::lock_guard< mutex::Mutex > lock( connections_lock_ );
      connections_[connection->handle()] = wptr;
    }
    
    ptr->SetConnectionManager( shared_from_this() );

    return connection;
  }

  std::size_t size() const 
  {
    std::size_t ret;
    {
      std::lock_guard< mutex::Mutex > lock( connections_lock_ );
      ret = connections_.size();
    }

    return ret;
    
  }

  void Leave(connection_handle_type const &id) override 
  {
    
    std::lock_guard< mutex::Mutex > lock( connections_lock_ );
    auto it =connections_.find( id );
    
    if( it != connections_.end() ) {
      connections_.erase(it);
    }

  }

  void Enter(weak_connection_type wptr) 
  {
    auto ptr = wptr.lock();
    if(ptr) {
      std::lock_guard< mutex::Mutex > lock( connections_lock_ );      
      connections_[ ptr->handle() ] = ptr;
      
    }
    
  }
  
  
private:
  mutable mutex::Mutex connections_lock_;
  std::unordered_map< connection_handle_type, weak_connection_type > connections_;
};

class ConnectionRegister 
{
public:
  typedef typename AbstractConnection::connection_handle_type connection_handle_type;
  typedef std::weak_ptr< AbstractConnection > weak_connection_type;
  typedef std::shared_ptr< ConnectionRegisterImpl > shared_implementation_pointer_type;

  ConnectionRegister () 
  {
    ptr_ = std::make_shared< ConnectionRegisterImpl >();
  }
  
  
  template< typename T, typename... Args >
  std::shared_ptr< T > CreateClient(Args &&...args) 
  {
    return ptr_->CreateClient< T, Args... >( std::forward<Args>( args )...  );
  }

  std::size_t size() const 
  {
    return ptr_->size();
  }

  void Enter(weak_connection_type wptr) 
  {
    ptr_->Enter(wptr);
  }

  
private:
  shared_implementation_pointer_type ptr_;
   
};

  

}
}
#endif
