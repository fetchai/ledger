#ifndef NETWORK_ABSTRACT_CONNECTION_HPP
#define NETWORK_ABSTRACT_CONNECTION_HPP
#include "core/mutex.hpp"
#include "network/message.hpp"
#include "network/tcp/abstract_connection_register.hpp"

#include <memory>
#include<atomic>
namespace fetch {
namespace network {

class AbstractConnection : public std::enable_shared_from_this<AbstractConnection> {
 public:
  typedef std::shared_ptr<AbstractConnection> shared_type;
  typedef typename AbstractConnectionRegister::connection_handle_type connection_handle_type;
  typedef std::weak_ptr< AbstractConnection > weak_ptr_type;  
  typedef std::weak_ptr< AbstractConnectionRegister > weak_register_type;
  
  enum {
    TYPE_UNDEFINED = 0,
    TYPE_INCOMING = 1,
    TYPE_OUTGOING = 2
  } ;
  
  AbstractConnection() 
  {
    handle_ = AbstractConnection::next_handle();
  }

  // Interface
  virtual ~AbstractConnection() {
    auto ptr = connection_register_.lock();
    if(ptr) {
      ptr->Leave( handle_ );      
    }    
  }
  
  virtual void Send(message_type const&) = 0;

  virtual std::string Address() = 0;
  virtual uint16_t Type() const = 0;

  // Common to all
  connection_handle_type handle() const noexcept { return handle_; }
  void SetConnectionManager(weak_register_type const &reg) 
  {
    connection_register_ = reg;
  }

  weak_ptr_type network_client_pointer() 
  {
    return shared_from_this();
  }
  
 private:
  static connection_handle_type next_handle() {
    std::lock_guard<fetch::mutex::Mutex> lck(global_handle_mutex_);
    connection_handle_type ret = global_handle_counter_;
    ++global_handle_counter_;
    return ret;
  }

  weak_register_type connection_register_;  
  std::atomic< connection_handle_type > handle_;
  
  static connection_handle_type global_handle_counter_;
  static fetch::mutex::Mutex global_handle_mutex_;
  
};

AbstractConnection::connection_handle_type
    AbstractConnection::global_handle_counter_ = 0;
fetch::mutex::Mutex AbstractConnection::global_handle_mutex_(__LINE__,
                                                                __FILE__);
}
}

#endif
