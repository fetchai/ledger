#ifndef NETWORK_TCP_CLIENT_HPP
#define NETWORK_TCP_CLIENT_HPP

#include "byte_array/const_byte_array.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "logger.hpp"
#include "network/message.hpp"
#include "network/tcp/client_implementation.hpp"
#include "network/thread_manager.hpp"
#include "serializers/byte_array_buffer.hpp"
#include "serializers/referenced_byte_array.hpp"

#include "mutex.hpp"

#include "fetch_asio.hpp"

#include <atomic>
#include <memory>
#include <mutex>

namespace fetch {
namespace network {

class TCPClient {
 public:
  typedef ThreadManager thread_manager_type;

  typedef typename ThreadManager::event_handle_type event_handle_type;
  typedef uint64_t handle_type;
  typedef TCPClientImplementation implementation_type;  
  typedef std::shared_ptr< implementation_type > pointer_type;
  
  TCPClient(byte_array::ConstByteArray const& host,
            byte_array::ConstByteArray const& port,
            thread_manager_type &thread_manager) noexcept
  {
    try {
      pointer_ = std::make_shared< implementation_type >(thread_manager);
      pointer_->OnConnectionFailed([this]() { this->ConnectionFailed(); });
      pointer_->OnPushMessage([this](message_type const& m) { this->PushMessage(m); });
      pointer_->Connect(host, port);
      
    } catch( ... ) { // std::__1::system_error: kqueue: Too many open files
      pointer_.reset();
    }
  }

  TCPClient(byte_array::ConstByteArray const& host, uint16_t const& port,
            thread_manager_type &thread_manager) noexcept
  {
    try {
      pointer_ = std::make_shared< implementation_type >(thread_manager);
      pointer_->OnConnectionFailed([this]() { this->ConnectionFailed(); });
      pointer_->OnPushMessage([this](message_type const& m) { this->PushMessage(m); });    
      
      pointer_->Connect(host, port);
    } catch( ... ) { // // std::__1::system_error: kqueue: Too many open files
      pointer_.reset();
    }

  }

  virtual ~TCPClient() noexcept {    
    LOG_STACK_TRACE_POINT;
    pointer_.reset();    
  }

  void Close() {
    if(pointer_) {
      pointer_->ClearConnectionFailed();
      pointer_->ClearPushMessage();
      pointer_->ClearLeave();
    }
  }

  void OnConnectionFailed(std::function< void() > const &fnc)
  {
    if(pointer_) {
      pointer_->OnConnectionFailed( std::move(fnc) );
    }
  }

  void OnPushMessage(std::function< void(message_type const&) > const &fnc)
  {
    if(pointer_) {
      pointer_->OnPushMessage( std::move(fnc) );
    }
  }

  void OnLeave(std::function<void()> &&fnc) {
    if(pointer_) {
      pointer_->OnLeave( std::move(fnc) );
    }
  }
  
  

  void Send(message_type const& msg) noexcept {
    if(pointer_) {
      pointer_->Send(msg);     
    }
    // TODO: Throw exception if it does not have a pointer?
  }

  virtual void PushMessage(message_type const& value) = 0;
  virtual void ConnectionFailed() = 0;

  handle_type const& handle() const noexcept { return pointer_->handle(); }

  std::string Address() const noexcept {
    if(pointer_) {
      return pointer_->Address();
    }
    return "";
  }

  void OnLeave(std::function<void()> fnc) {
    if(pointer_) {
      pointer_->OnLeave(fnc);
    }
  }

  bool is_alive() const noexcept {
    if(pointer_) {
      return pointer_->is_alive();
    }
    return false;
  }

private:
  pointer_type pointer_;
  
};
  
}
}

#endif
