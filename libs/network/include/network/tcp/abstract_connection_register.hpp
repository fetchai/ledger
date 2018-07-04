#ifndef NETWORK_ABSTRACT_CONNECTION_REGISTER_HPP
#define NETWORK_ABSTRACT_CONNECTION_REGISTER_HPP

#include<memory>

namespace fetch {
namespace network {

class AbstractConnection;

class AbstractConnectionRegister : public std::enable_shared_from_this< AbstractConnectionRegister >
{
public:
  typedef uint64_t connection_handle_type;
  AbstractConnectionRegister() = default;
  AbstractConnectionRegister(AbstractConnectionRegister const &other) = delete;
  AbstractConnectionRegister(AbstractConnectionRegister &&other) = default;  
  AbstractConnectionRegister& operator=(AbstractConnectionRegister const &other) = delete;  
  AbstractConnectionRegister& operator=(AbstractConnectionRegister &&other) = default;
    
  virtual ~AbstractConnectionRegister() = default;
  
  virtual void Leave(connection_handle_type const &id) = 0;
  virtual void Enter(std::weak_ptr< AbstractConnection> const &) = 0;  
  
};


}
}

#endif
