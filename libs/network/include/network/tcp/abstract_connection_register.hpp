#ifndef NETWORK_ABSTRACT_CONNECTION_REGISTER_HPP
#define NETWORK_ABSTRACT_CONNECTION_REGISTER_HPP

#include"core/mutex.hpp"
#include<memory>
#include<unordered_map>

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
  typedef std::unordered_map< connection_handle_type, weak_service_client_type > service_map_type;
  
  AbstractConnectionRegister() = default;
  AbstractConnectionRegister(AbstractConnectionRegister const &other) = delete;
  AbstractConnectionRegister(AbstractConnectionRegister &&other) = default;  
  AbstractConnectionRegister& operator=(AbstractConnectionRegister const &other) = delete;  
  AbstractConnectionRegister& operator=(AbstractConnectionRegister &&other) = default;
    
  virtual ~AbstractConnectionRegister() = default;
  
  virtual void Leave(connection_handle_type const &id) = 0;
  virtual void Enter(std::weak_ptr< AbstractConnection> const &) = 0;  

  weak_service_client_type GetService(connection_handle_type const &i) 
  {
    std::lock_guard< mutex::Mutex > lock( service_lock_ );
    return services_[i];
  }  
  
  void WithServices(std::function< void(service_map_type const &) > f) const
  {
    std::lock_guard< mutex::Mutex > lock( service_lock_ );
    f(services_);
  }
protected:
  void RemoveService(connection_handle_type const &n) 
  {
    std::lock_guard< mutex::Mutex > lock( service_lock_ );
    auto it = services_.find(n);
    if(it != services_.end()) services_.erase(it);   
  }

  void AddService(connection_handle_type const &n, weak_service_client_type const &ptr) 
  {
    std::lock_guard< mutex::Mutex > lock( service_lock_ );
    services_[n] = ptr;
    
  }
  
private:
  mutable mutex::Mutex service_lock_;
  service_map_type services_;      
};


}
}

#endif
