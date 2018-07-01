#ifndef NETWORK_ABSTRACT_CONNECTION_HPP
#define NETWORK_ABSTRACT_CONNECTION_HPP
#include "core/mutex.hpp"
#include "network/message.hpp"

#include <memory>
#include<atomic>
namespace fetch {
namespace network {

class AbstractConnection {
 public:
  typedef std::shared_ptr<AbstractConnection> shared_type;
  typedef uint64_t connection_handle_type;
  enum {
    TYPE_UNDEFINED = 0,
    TYPE_INCOMING = 1,
    TYPE_OUTGOING = 2
  } ;
  AbstractConnection() 
  {
    handle_ = AbstractConnection::next_handle();
  }
  
  virtual ~AbstractConnection() {}
  virtual void Send(message_type const&) = 0;

  virtual std::string Address() = 0;
  virtual uint16_t Type() const = 0;

  connection_handle_type handle() const noexcept { return handle_; }
 private:
  static connection_handle_type next_handle() {
    std::lock_guard<fetch::mutex::Mutex> lck(global_handle_mutex_);
    connection_handle_type ret = global_handle_counter_;
    ++global_handle_counter_;
    return ret;
  }

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
