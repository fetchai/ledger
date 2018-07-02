#ifndef NETWORK_TCP_CLIENT_IMPLEMENTATION_HPP
#define NETWORK_TCP_CLIENT_IMPLEMENTATION_HPP

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/referenced_byte_array.hpp"
#include "core/logger.hpp"
#include "network/message.hpp"
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

class TCPClientImplementation final : public std::enable_shared_from_this<TCPClientImplementation>
{
 public:
  typedef ThreadManager                              thread_manager_type;
  typedef uint64_t                                   handle_type;
  typedef std::weak_ptr<TCPClientImplementation>     self_type;
  typedef std::shared_ptr<TCPClientImplementation>   shared_self_type;
  typedef asio::ip::tcp::tcp::socket                 socket_type;
  typedef asio::ip::tcp::resolver                    resolver_type;


  TCPClientImplementation(thread_manager_type &thread_manager) noexcept :
    threadManager_{thread_manager}
  {
    LOG_STACK_TRACE_POINT;
    handle_ = next_handle();
  }

  TCPClientImplementation(TCPClientImplementation const &rhs)            = delete;
  TCPClientImplementation(TCPClientImplementation &&rhs)                 = delete;
  TCPClientImplementation &operator=(TCPClientImplementation const &rhs) = delete;
  TCPClientImplementation &operator=(TCPClientImplementation&& rhs)      = delete;
  ~TCPClientImplementation() { }

  void Connect(byte_array::ConstByteArray const& host, uint16_t port)
  {
    Connect(host, byte_array::ConstByteArray(std::to_string(port)));
  }

  void Connect( byte_array::ConstByteArray const& host,
    byte_array::ConstByteArray const& port)
  {
    self_type self = shared_from_this();
    fetch::logger.Debug("Client posting connect");
    threadManager_.Post([this, self, host, port]
    {
      shared_self_type selfLock = self.lock();
      if(!selfLock) return;

      // We get a weak socket from the thread manager. This must only be strong in the TM queue
      std::shared_ptr<socket_type> socket = threadManager_.CreateIO<socket_type>();
      socket_ = socket;

      std::shared_ptr<resolver_type> res = threadManager_.CreateIO<resolver_type>();

      auto cb = [this, self, res, socket]
        (std::error_code ec, resolver_type::iterator)
      {
        shared_self_type selfLock = self.lock();
        if(!selfLock) return;

        LOG_STACK_TRACE_POINT;
        fetch::logger.Info("Finished connecting.");
        if (!ec)
        {
          fetch::logger.Debug("Connection established!");
          ReadHeader();
        } else
        {
          fetch::logger.Debug("Client failed to connect");
          ConnectionFailed();
        }
      };

      if(socket && res)
      {
        resolver_type::iterator it
          (res->resolve({std::string(host), std::string(port)}));
        asio::async_connect(*socket, it, cb);
      } else {
        fetch::logger.Error("Failed to create valid socket");
      }
    });
  }

  // TODO: (`HUT`) : check this is safe, possibly not
  std::string Address() const noexcept
  {
    auto socket = socket_.lock();
    if(!socket)
    {
      fetch::logger.Error("Attempting to get Address of dead socket. Returning.");
      return "";
    }

    return (*socket).remote_endpoint().address().to_string();
  }

  handle_type const& handle() const noexcept { return handle_; }

  bool is_alive() const noexcept
  {
    return !socket_.expired() && connected_;
  }

  void Send(message_type const& msg) noexcept
  {
    if(!connected_)
    {
      fetch::logger.Error("Attempting to write to socket too early. Returning.");
      return;
    }

    {
      std::lock_guard<fetch::mutex::Mutex> lock(queue_mutex_);
      write_queue_.push_back(msg);
    }

     self_type self = shared_from_this();
     threadManager_.Post([this, self]
     {
      shared_self_type selfLock = self.lock();
      if(!selfLock) return;

       WriteNext();
     });
  }

  void Close() noexcept
  {
    std::weak_ptr<socket_type> socketWeak = socket_;

    threadManager_.Post([socketWeak]
      {
        auto socket = socketWeak.lock();
        if(socket)
        {
          std::error_code dummy;
          socket->shutdown(asio::ip::tcp::socket::shutdown_both, dummy);
          socket->close(dummy);
        }
      });
  }

  bool Closed() noexcept
  {
    return socket_.expired();
  }

  void OnConnectionFailed(std::function< void() > const &fnc)
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
    on_connection_failed_ = fnc;
  }

  void OnPushMessage(std::function< void(message_type const&) > const &fnc)
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
    on_push_message_ = fnc;
  }

  void ClearClosures() noexcept
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
    on_connection_failed_  = nullptr;
    on_push_message_  = nullptr;
  }

 private:
  static const uint64_t networkMagic = 0xFE7C80A1FE7C80A1;

  thread_manager_type threadManager_;
  // socket is guaranteed to have lifetime less than the io_service/threadManager
  std::weak_ptr<socket_type>      socket_;

  message_queue_type          write_queue_;
  mutable fetch::mutex::Mutex queue_mutex_;
  mutable fetch::mutex::Mutex write_mutex_;

  mutable fetch::mutex::Mutex                callback_mutex_;
  std::function< void(message_type const&) > on_push_message_;
  std::function< void() >                    on_connection_failed_;
  std::function<void()>                      on_leave_;

  bool                       connected_{false};

  handle_type handle_;
  static handle_type global_handle_counter_;
  static fetch::mutex::Mutex global_handle_mutex_;

  static handle_type next_handle()
  {
    std::lock_guard<fetch::mutex::Mutex> lck(global_handle_mutex_);
    handle_type ret = global_handle_counter_;
    ++global_handle_counter_;
    return ret;
  }

  union {
    char bytes[2 * sizeof(uint64_t)];
    struct {
      uint64_t magic;
      uint64_t length;
    } content;

  } header_;

  void ReadHeader() noexcept
  {
    LOG_STACK_TRACE_POINT;
    self_type self = shared_from_this();
    auto socket = socket_.lock();
    byte_array::ByteArray header;

    // TODO: (`HUT`) : fix. the requirement for strong self here
    auto cb = [this, self, socket, header]
      (std::error_code ec, std::size_t) {
      shared_self_type selfLock = self.lock();
      if(!selfLock) return;

      if (!ec)
      {
        fetch::logger.Debug("Read message header.");
        ReadBody();
      } else {
        // We expect to get an ec here when the socked is closed via a post
      }
    };

    if(socket)
    {
      asio::async_read(*socket, asio::buffer(this->header_.bytes, 2 * sizeof(uint64_t)), cb);
      connected_ = true;
    }
  }

  void ReadBody() noexcept
  {
    if (header_.content.magic != networkMagic)
    {
      fetch::logger.Error("Magic incorrect during network read - dying: ", header_.content.magic);
      return;
    }

    byte_array::ByteArray message;
    message.Resize(header_.content.length);

    self_type self = shared_from_this();
    auto socket = socket_.lock();
    auto cb = [this, self, message, socket](std::error_code ec, std::size_t len) {

      shared_self_type selfLock = self.lock();
      if(!selfLock) return;

      if (!ec)
      {
        PushMessage(message);
        ReadHeader();
      } else
      {
        fetch::logger.Error("Reading body failed, dying: ", ec);
      }
    };

    if(socket)
    {
      asio::async_read(*socket, asio::buffer(message.pointer(), message.size()), cb);
    }
  }

  static void SetHeader(byte_array::ByteArray &header, uint64_t bufSize)
  {
    header.Resize(16);

    for (std::size_t i = 0; i < 8; ++i)
    {
      header[i] = uint8_t((networkMagic >> i*8) & 0xff);
    }

    for (std::size_t i = 0; i < 8; ++i)
    {
      header[i+8] = uint8_t((bufSize >> i*8) & 0xff);
    }
  }

  // Always executed in a run()
  void WriteNext()
  {
    // Only one thread can get past here at a time
    if(!write_mutex_.try_lock())
    {
      return;
    }

    self_type self = shared_from_this();
    auto socket = socket_.lock();

    message_type buffer;
    {
      std::lock_guard<fetch::mutex::Mutex> lock(queue_mutex_);
      if(write_queue_.empty()) { write_mutex_.unlock(); return; }
      buffer = write_queue_.front();
      write_queue_.pop_front();
    }

    byte_array::ByteArray header;
    SetHeader(header, buffer.size());

    std::vector<asio::const_buffer> buffers{
      asio::buffer(header.pointer(), header.size()),
      asio::buffer(buffer.pointer(), buffer.size())
    };

    auto cb = [this, self, socket, buffer, header](std::error_code ec, std::size_t len)
    {
        shared_self_type selfLock = self.lock();
        if(!selfLock) return;

        write_mutex_.unlock();

        if(ec)
        {
          fetch::logger.Error("Error writing to socket, closing.");
          return;
        }

        WriteNext();
    };

    //auto socket = socket_.lock();
    if(socket)
    {
      asio::async_write(*socket, buffers, cb);
    } else {
      fetch::logger.Error("Failed to lock socket in WriteNext!");
    }
  }

  void ConnectionFailed()
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
    if(on_connection_failed_) on_connection_failed_();
      fetch::logger.Error("Foo!");
  }

  void PushMessage(message_type message)
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
    if(on_push_message_) on_push_message_(message);
  }
};

TCPClientImplementation::handle_type TCPClientImplementation::global_handle_counter_ = 0;
fetch::mutex::Mutex TCPClientImplementation::global_handle_mutex_(__LINE__, __FILE__);

}
}
#endif
