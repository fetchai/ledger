#pragma once

#include"core/mutex.hpp"
#include<memory>
#include<unordered_map>
#include<atomic>

namespace fetch {
namespace service {
class ServiceClient;
}

namespace network {
class AbstractConnection;

class AbstractConnectionRegister : public std::enable_shared_from_this< AbstractConnectionRegister >
{
public:
  typedef uint64_t connection_handle_type;
  typedef std::weak_ptr< service::ServiceClient > weak_service_client_type;
  typedef std::shared_ptr< service::ServiceClient > shared_service_client_type;  
  typedef std::unordered_map< connection_handle_type, weak_service_client_type > service_map_type;
  
  AbstractConnectionRegister() 
  {
    number_of_services_ = 0;    
  }
  
  AbstractConnectionRegister(AbstractConnectionRegister const &other) = delete;
  AbstractConnectionRegister(AbstractConnectionRegister &&other) = default;  
  AbstractConnectionRegister& operator=(AbstractConnectionRegister const &other) = delete;  
  AbstractConnectionRegister& operator=(AbstractConnectionRegister &&other) = default;
    
  virtual ~AbstractConnectionRegister() = default;
  
  virtual void Leave(connection_handle_type const &id) = 0;
  virtual void Enter(std::weak_ptr< AbstractConnection> const &) = 0;  

  shared_service_client_type GetService(connection_handle_type const &i) 
  {
    std::lock_guard< mutex::Mutex > lock( service_lock_ );
    if(services_.find(i) == services_.end()) return nullptr;
    return services_[i].lock();
  }  
  
  void WithServices(std::function< void(service_map_type const &) > f) const
  {
    std::lock_guard< mutex::Mutex > lock( service_lock_ );
    f(services_);
  }
  
  uint64_t number_of_services() const 
  {
    return number_of_services_;
  }
  
protected:
  void RemoveService(connection_handle_type const &n) 
  {
    --number_of_services_;
    std::lock_guard< mutex::Mutex > lock( service_lock_ );
    auto it = services_.find(n);
    if(it != services_.end()) services_.erase(it);   
  }

  void AddService(connection_handle_type const &n, weak_service_client_type const &ptr) 
  {
    ++number_of_services_;
    
    std::lock_guard< mutex::Mutex > lock( service_lock_ );
    services_[n] = ptr;
  }

  template< typename T >
  void ActivateSelfManage(T ptr) 
  {
    ptr->ActivateSelfManage();
  }
 
  template< typename T >
  void DeactivateSelfManage(T ptr) 
  {
    ptr->ADativateSelfManage();
  }
 
  
private:
  mutable mutex::Mutex service_lock_;
  service_map_type services_;
  std::atomic< uint64_t > number_of_services_;
  
};


}
}

