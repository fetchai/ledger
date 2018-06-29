#ifndef NETWORK_TCP_CLIENT_HPP
#define NETWORK_TCP_CLIENT_HPP

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/referenced_byte_array.hpp"
#include "core/logger.hpp"
#include "network/message.hpp"
#include "network/tcp/client_implementation.hpp"
#include "network/details/thread_manager.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/referenced_byte_array.hpp"

#include "core/mutex.hpp"

#include "network/fetch_asio.hpp"

#include <atomic>
#include <memory>
#include <mutex>

namespace fetch
{
namespace network
{

class TCPClient
{
 public:
  typedef ThreadManager                             thread_manager_type;
  typedef typename ThreadManager::event_handle_type event_handle_type;
  typedef uint64_t                                  handle_type;
  typedef TCPClientImplementation                   implementation_type;
  typedef std::shared_ptr<implementation_type>      pointer_type;

  explicit TCPClient(thread_manager_type &thread_manager)
    : pointer_{std::make_shared< implementation_type >(thread_manager)}
  {
    RegisterHandlers();
  }

  // Policy: copy and move constructors, the last client will be the one connected
  TCPClient(TCPClient const &rhs)
  {
    pointer_ = rhs.pointer_;
    rhs.pointer_ = nullptr; // avoid having other client clearing our closures
    Cleanup();
    RegisterHandlers();
  }

  TCPClient(TCPClient &&rhs)
  {
    pointer_ = rhs.pointer_;
    rhs.pointer_ = nullptr; // avoid having other client clearing our closures
    Cleanup();
    RegisterHandlers();
  }

  TCPClient &operator=(TCPClient const &rhs)
  {
    pointer_ = rhs.pointer_;
    rhs.pointer_ = nullptr; // avoid having other client clearing our closures
    Cleanup();
    RegisterHandlers();
    return *this;
  }

  TCPClient &operator=(TCPClient&& rhs)
  {
    pointer_ = rhs.pointer_;
    rhs.pointer_ = nullptr; // avoid having other client clearing our closures
    Cleanup();
    RegisterHandlers();
    return *this;
  }

  virtual ~TCPClient() noexcept {
    LOG_STACK_TRACE_POINT;

    if(pointer_)
    {
      Cleanup();
      pointer_->Close();
      pointer_.reset();
    }
  }

  void Connect(byte_array::ConstByteArray const& host, uint16_t port)
  {
    if (pointer_)
    {
      pointer_->Connect(host, port);
    }
  }

  void Connect(byte_array::ConstByteArray const& host, byte_array::ConstByteArray const& port)
  {
    if (pointer_)
    {
      pointer_->Connect(host, port);
    }
  }

  // For safety, this MUST be called by the base class in its destructor
  // As closures to that class exist in the client implementation
  void Cleanup()
  {
    if(pointer_)
    {
      pointer_->ClearClosures();
    }
  }

  void Send(message_type const& msg) noexcept
  {
    pointer_->Send(msg);
  }

  virtual void PushMessage(message_type const& value) = 0;
  virtual void ConnectionFailed() = 0;

  handle_type const& handle() const noexcept { return pointer_->handle(); }

  std::string Address() const noexcept
  {
    return pointer_->Address();
  }

  bool is_alive() const noexcept { return pointer_->is_alive(); }

protected:

  mutable pointer_type  pointer_;

  void RegisterHandlers()
  {
    pointer_->OnConnectionFailed(
      [this]() {
        this->ConnectionFailed();
      });

    pointer_->OnPushMessage(
      [this](message_type const &m) {
        this->PushMessage(m);
      });
  }
};

}
}

#endif
