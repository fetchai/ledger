#ifndef NETWORK_TCP_CLIENT_HPP
#define NETWORK_TCP_CLIENT_HPP

#include "byte_array/const_byte_array.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "logger.hpp"
#include "network/message.hpp"
#include "network/tcp/client_implementation.hpp"
#include "network/thread_manager.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/referenced_byte_array.hpp"

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
  typedef thread_manager_type* thread_manager_ptr_type;
  typedef typename ThreadManager::event_handle_type event_handle_type;
  typedef uint64_t handle_type;
  typedef TCPClientImplementation implementation_type;  
  typedef std::shared_ptr< implementation_type > pointer_type;
  
  TCPClient(byte_array::ConstByteArray const& host,
            byte_array::ConstByteArray const& port,
            thread_manager_ptr_type thread_manager) noexcept
  {
    pointer_ = std::make_shared< implementation_type >(thread_manager);
    pointer_->OnConnectionFailed([this]() { this->ConnectionFailed(); });
    pointer_->OnPushMessage([this](message_type const& m) { this->PushMessage(m); });    
    
    pointer_->Connect(host, port);
  }

  TCPClient(byte_array::ConstByteArray const& host, uint16_t const& port,
            thread_manager_ptr_type thread_manager) noexcept
  {
    pointer_ = std::make_shared< implementation_type >(thread_manager);
    pointer_->OnConnectionFailed([this]() { this->ConnectionFailed(); });
    pointer_->OnPushMessage([this](message_type const& m) { this->PushMessage(m); });    

    pointer_->Connect(host, port);    
  }

  virtual ~TCPClient() noexcept {    
    LOG_STACK_TRACE_POINT;
    std::cout << "Destruct 1" << std::endl;
    
    pointer_->ClearConnectionFailed();
    pointer_->ClearPushMessage();
    pointer_->ClearLeave();
    pointer_->Close();
    
    pointer_.reset();    
  }

  void Send(message_type const& msg) noexcept {
    pointer_->Send(msg);
  }

  virtual void PushMessage(message_type const& value) = 0;
  virtual void ConnectionFailed() = 0;

  handle_type const& handle() const noexcept { return pointer_->handle(); }

  std::string Address() const noexcept {
    return pointer_->Address();
  }

  void OnLeave(std::function<void()> fnc) {
    pointer_->OnLeave(fnc);
  }

  bool is_alive() const noexcept { return pointer_->is_alive(); }

private:
  pointer_type pointer_;
  
};
  
}
}

#endif
