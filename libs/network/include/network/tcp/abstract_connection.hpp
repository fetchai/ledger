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
    {
      std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);          
      on_message_ = nullptr;
    }
    
    auto ptr = connection_register_.lock();
    if(ptr) {
      ptr->Leave( handle_ );      
    }    
  }
  
  virtual void Send(message_type const&) = 0;
  virtual uint16_t Type() const = 0;
  virtual void Close() = 0;
  virtual bool Closed() = 0;
  virtual bool is_alive() const = 0;
  
  // Common to all
  std::string Address() const
  {
    std::lock_guard< mutex::Mutex > lock(address_mutex_);
    return address_;
  }
  
  connection_handle_type handle() const noexcept { return handle_; }
  void SetConnectionManager(weak_register_type const &reg) 
  {
    connection_register_ = reg;
  }

  shared_type connection_pointer() 
  {
    return shared_from_this();
  }

  void OnMessage(std::function< void(network::message_type const& msg) > const &f) 
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);    
    on_message_ = f;    
  }

  void OnConnectionFailed(std::function< void() > const &fnc)
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
    on_connection_failed_ = fnc;
  }
  
  void ClearClosures() noexcept
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
    on_connection_failed_  = nullptr;
    on_message_  = nullptr;
  }
  
protected:
  void SetAddress(std::string const &addr) 
  {
    std::lock_guard< mutex::Mutex > lock(address_mutex_);    
    address_ = addr;
  }


  void SignalMessage(network::message_type const& msg) 
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);    
    if(on_message_) on_message_(msg);
  }
  
  void SignalConnectionFailed()
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
    if(on_connection_failed_) on_connection_failed_();
  }

  
 private:
  std::function< void(network::message_type const& msg) > on_message_;
  std::function< void() >                    on_connection_failed_;
  
  std::string address_;  
  mutable mutex::Mutex address_mutex_;
  
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

  mutable fetch::mutex::Mutex                callback_mutex_;  
};


}
}

#endif
