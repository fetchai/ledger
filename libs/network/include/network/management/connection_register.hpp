#ifndef NETWORK_CONNECTION_MANAGER_HPP
#define NETWORK_CONNECTION_MANAGER_HPP
#include "network/management/abstract_connection_register.hpp"
#include "network/service/client.hpp"
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
  typedef service::ServiceClient service_client_type;
  typedef std::shared_ptr< service::ServiceClient > shared_service_client_type;
  typedef std::weak_ptr< service::ServiceClient > weak_service_client_type;  
  typedef G details_type;  
  struct  LockableDetails final : public details_type, public mutex::Mutex {  };
  typedef std::unordered_map< connection_handle_type, std::shared_ptr< LockableDetails > >  details_map_type;
  typedef std::function< void(connection_handle_type) > callback_client_enter_type;
  typedef std::function< void(connection_handle_type) > callback_client_leave_type;

  
  ConnectionRegisterImpl() = default;
  ConnectionRegisterImpl(ConnectionRegisterImpl const &other) = delete;
  ConnectionRegisterImpl(ConnectionRegisterImpl &&other) = default;  
  ConnectionRegisterImpl& operator=(ConnectionRegisterImpl const &other) = delete;
  ConnectionRegisterImpl& operator=(ConnectionRegisterImpl &&other) = default;
  
  virtual ~ConnectionRegisterImpl() = default;
      
  template< typename T,  typename... Args >
  shared_service_client_type CreateServiceClient(NetworkManager const &tm, Args &&...args)
  {

    T connection(tm);
    connection.Connect( std::forward<Args>(args)... );

    shared_service_client_type service = std::make_shared< service_client_type >(connection.connection_pointer().lock(), tm );
    

    auto wptr = connection.connection_pointer();    
    auto ptr = wptr.lock();
    assert( ptr );

    Enter(wptr);
    ptr->SetConnectionManager( shared_from_this() );

    {
      std::lock_guard< mutex::Mutex > lock( connections_lock_ );      
      AddService( ptr->handle(), service);
    }
    
    return service;
  }

  std::size_t size() const 
  {
    std::lock_guard< mutex::Mutex > lock( connections_lock_ );
    return  connections_.size();
  }

  
  void OnClientEnter(callback_client_enter_type const &f) 
  {
    on_client_enter_ = f;
  }

  void OnClientLeave(callback_client_enter_type const &f) 
  {
    on_client_leave_ = f;
  }

  
  void Leave(connection_handle_type const &id) override 
  {
    {      
      RemoveService(id);
    
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

    SignalClientLeave( id );
  }

  void Enter(weak_connection_type  const &wptr) override
  {
    connection_handle_type handle = connection_handle_type(-1);
    
    {
      auto ptr = wptr.lock();
      if(ptr) {
        std::lock_guard< mutex::Mutex > lock( connections_lock_ );
        connections_[ ptr->handle() ] = ptr;
        details_[ ptr->handle() ] = std::make_shared<LockableDetails>();
        handle =  ptr->handle();
      }
    }
    
    if(handle != connection_handle_type(-1)) {
      SignalClientEnter(handle );
    }
    
  }

  
  std::shared_ptr< LockableDetails > GetDetails(connection_handle_type const &i) 
  {
    std::lock_guard< mutex::Mutex > lock( details_lock_ );
    return details_[i];
  }

  void WithClientDetails(std::function< void(details_map_type const &) > fnc) const
  {
    std::lock_guard< mutex::Mutex > lock( details_lock_ );
    fnc(details_);
  }


  void WithClientDetails(std::function< void(details_map_type &) > fnc) 
  {
    std::lock_guard< mutex::Mutex > lock( details_lock_ );
    fnc(details_);
  }
  
private:
  mutable mutex::Mutex connections_lock_;
  std::unordered_map< connection_handle_type, weak_connection_type > connections_;

  mutable mutex::Mutex details_lock_;
  std::unordered_map< connection_handle_type, std::shared_ptr< LockableDetails > >  details_;

  
  void SignalClientLeave(connection_handle_type const&handle) 
  {
    if(on_client_leave_) on_client_leave_(handle);
  }
  
  void SignalClientEnter(connection_handle_type const&handle) 
  {
    if(on_client_enter_) on_client_enter_(handle);
  }

  callback_client_enter_type on_client_leave_;    
  callback_client_enter_type on_client_enter_;
    
};


template< typename G >
class ConnectionRegister 
{
public:
  typedef typename AbstractConnection::connection_handle_type connection_handle_type;
  typedef std::weak_ptr< AbstractConnection > weak_connection_type;
  typedef std::shared_ptr< ConnectionRegisterImpl<G> > shared_implementation_pointer_type;
  typedef typename ConnectionRegisterImpl<G>::LockableDetails lockable_details_type;
  typedef std::shared_ptr< service::ServiceClient > shared_service_client_type;
  typedef std::weak_ptr< service::ServiceClient > weak_service_client_type;
  typedef AbstractConnectionRegister::service_map_type  service_map_type;
  typedef typename ConnectionRegisterImpl<G>::details_map_type details_map_type;
  typedef std::function< void(connection_handle_type) > callback_client_enter_type;
  typedef std::function< void(connection_handle_type) > callback_client_leave_type;
  
  ConnectionRegister () 
  {
    ptr_ = std::make_shared< ConnectionRegisterImpl<G> >();
  }
    
  template< typename T, typename... Args >
  shared_service_client_type CreateServiceClient(NetworkManager const &tm, Args &&...args)
  {
    return ptr_->template CreateServiceClient< T, Args... >(tm, std::forward<Args>( args )...  );
  }

  std::size_t size() const 
  {
    return ptr_->size();
  }

  void OnClientEnter(callback_client_enter_type const &f) 
  {
    return ptr_->OnClientEnter(f);    
  }

  void OnClientLeave(callback_client_enter_type const &f) 
  {
    return ptr_->OnClientLeave(f);
  }
  
  std::shared_ptr< lockable_details_type > GetDetails(connection_handle_type const &i) 
  {
    return ptr_->GetDetails(i);
  }

  weak_service_client_type GetService(connection_handle_type &&i) 
  {
    return ptr_->GetService(std::move(i));
  }  
  
  
  void WithServices(std::function< void(service_map_type const &) > const &f) const
  {
    ptr_->WithServices(f);
  }

  void WithClientDetails(std::function< void(details_map_type const &) > fnc) const
  {
    ptr_->WithClientDetails(fnc);   
  }

  void WithClientDetails(std::function< void(details_map_type &) > fnc) 
  {
    ptr_->WithClientDetails(fnc);
  }

  
  uint64_t number_of_services() const 
  {
    return ptr_->number_of_services();
  }
  
  shared_implementation_pointer_type pointer() { return ptr_; }  
private:
  shared_implementation_pointer_type ptr_;
   
};

  

}
}
#endif
