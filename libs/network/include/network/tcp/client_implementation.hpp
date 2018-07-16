#ifndef NETWORK_TCP_CLIENT_IMPLEMENTATION_HPP
#define NETWORK_TCP_CLIENT_IMPLEMENTATION_HPP

#include "core/byte_array/encoders.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/referenced_byte_array.hpp"
#include "core/logger.hpp"
#include "network/message.hpp"
#include "network/management/network_manager.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/referenced_byte_array.hpp"

#include "core/mutex.hpp"
#include "network/management/abstract_connection.hpp"
#include "network/fetch_asio.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <atomic>

namespace fetch
{
namespace network
{

class TCPClientImplementation final :
    public AbstractConnection
{
 public:
  typedef NetworkManager                      network_manager_type;
  typedef std::weak_ptr<AbstractConnection>   self_type;
  typedef std::shared_ptr<AbstractConnection> shared_self_type;
  typedef asio::ip::tcp::tcp::socket          socket_type;
  typedef asio::io_service::strand            strand_type;
  typedef asio::ip::tcp::resolver             resolver_type;
  //typedef fetch::mutex::Mutex                 mutex_type;

  TCPClientImplementation(network_manager_type &network_manager) noexcept :
    networkManager_{network_manager}
  {
  }

  TCPClientImplementation(TCPClientImplementation const &rhs)            = delete;
  TCPClientImplementation(TCPClientImplementation &&rhs)                 = delete;
  TCPClientImplementation &operator=(TCPClientImplementation const &rhs) = delete;
  TCPClientImplementation &operator=(TCPClientImplementation&& rhs)      = delete;
  ~TCPClientImplementation() { destructing_ = true; }

  void Connect(byte_array::ConstByteArray const& host, uint16_t port)
  {
    Connect(host, byte_array::ConstByteArray(std::to_string(port)));
  }

  void Connect( byte_array::ConstByteArray const& host,
    byte_array::ConstByteArray const& port)
  {
    self_type self = shared_from_this();

    fetch::logger.Debug("Client posting connect");

    networkManager_.Post([this, self, host, port]
    {
      shared_self_type selfLock = self.lock();
      if(!selfLock) return;

      // We get IO objects from the network manager, they will only be strong while in the post
      auto strand = networkManager_.CreateIO<strand_type>();
      if(!strand) return;
      {
        std::lock_guard<std::mutex> lock(io_creation_mutex_);
        strand_ = strand;
      }

      strand->post([this, self, host, port, strand]
      {
        shared_self_type selfLock = self.lock();
        if(!selfLock) return;

        std::shared_ptr<socket_type> socket = networkManager_.CreateIO<socket_type>();

        {
          std::lock_guard<std::mutex> lock(io_creation_mutex_);
          if(!postedClose_)
          {
            socket_ = socket;
          }
        }

        std::shared_ptr<resolver_type> res = networkManager_.CreateIO<resolver_type>();

        auto cb = [this, self, res, socket, strand]
        (std::error_code ec, resolver_type::iterator)
        {
          shared_self_type selfLock = self.lock();
          if(!selfLock) return;

          LOG_STACK_TRACE_POINT;
          fetch::logger.Info("Finished connecting.");
          if (!ec)
          {
            fetch::logger.Debug("Connection established!");
            this->SetAddress( (*socket).remote_endpoint().address().to_string() );

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

          assert(strand->running_in_this_thread());
          asio::async_connect(*socket, it, strand->wrap(cb));
        } else {
          fetch::logger.Error("Failed to create valid socket");
        }
      }); // end strand post
    }); // end NM post
  }

  bool is_alive() const noexcept
  {
    std::lock_guard<std::mutex> lock(io_creation_mutex_);
    return !socket_.expired() && connected_;
  }

  void Send(message_type const& msg) override
  {
    if(!connected_) // TODO: (`HUT`) : delete
    {
      fetch::logger.Warn("Attempting to write to socket too early. Returning.");
      return;
    }

    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      write_queue_.push_back(msg);
    }

     self_type self = shared_from_this();
     std::weak_ptr<strand_type> strand = strand_;

     networkManager_.Post([this, self, strand]
     {
      shared_self_type selfLock = self.lock();
      auto strandLock           = strand_.lock();
      if(!selfLock || !strandLock) return;

      strandLock->post([this, selfLock] { WriteNext(selfLock); });
     });
  }

  uint16_t Type() const override
  {
    return AbstractConnection::TYPE_OUTGOING;
  }

  void Close() noexcept
  {
    std::lock_guard<std::mutex> lock(io_creation_mutex_);
    postedClose_ = true;
    std::weak_ptr<socket_type> socketWeak = socket_;
    std::weak_ptr<strand_type> strandWeak = strand_;

    networkManager_.Post([socketWeak, strandWeak]
    {
      auto socket = socketWeak.lock();
      auto strand = strandWeak.lock();

      if(socket && strand)
      {
        strand->post([socket]
        {
          std::error_code dummy;
          socket->shutdown(asio::ip::tcp::socket::shutdown_both, dummy);
          socket->close(dummy);
        });
      }
    });
  }

  bool Closed() noexcept
  {
    return socket_.expired();
  }

  void OnConnectionFailed(std::function< void() > const &fnc)
  {
    std::lock_guard< std::mutex > lock(callback_mutex_);
    on_connection_failed_ = fnc;
  }

  void OnPushMessage(std::function< void(message_type const&) > const &fnc)
  {
    std::lock_guard< std::mutex > lock(callback_mutex_);
    on_push_message_ = fnc;
  }

  void ClearClosures() noexcept
  {
    std::lock_guard< std::mutex > lock(callback_mutex_);
    on_connection_failed_  = nullptr;
    on_push_message_  = nullptr;
  }

 private:
  static const uint64_t networkMagic_ = 0xFE7C80A1FE7C80A1;
  bool destructing_ = false;

  network_manager_type networkManager_;
  // IO objects should be guaranteed to have lifetime less than the io_service/networkManager
  std::weak_ptr<socket_type> socket_;
  std::weak_ptr<strand_type> strand_;

  message_queue_type          write_queue_;
  mutable std::mutex queue_mutex_;
  mutable std::mutex write_mutex_;
  mutable std::mutex io_creation_mutex_;
  bool                        postedClose_ = false;

  mutable std::mutex                callback_mutex_;
  std::function< void(message_type const&) > on_push_message_;
  std::function< void() >                    on_connection_failed_;
  std::function<void()>                      on_leave_;
  std::atomic<bool>                          connected_{false};

  void ReadHeader() noexcept
  {
    LOG_STACK_TRACE_POINT;
    auto strand = strand_.lock();
    if(!strand)
    {
      return;
    }
    assert(strand->running_in_this_thread());

    self_type self = shared_from_this();
    auto socket = socket_.lock();
    byte_array::ByteArray header;
    header.Resize(2 * sizeof(uint64_t));

    auto cb = [this, self, socket, header, strand]
      (std::error_code ec, std::size_t) {
      shared_self_type selfLock = self.lock();
      if(!selfLock) return;

      if (!ec)
      {
        fetch::logger.Debug("Read message header.");
        ReadBody(header);
      } else {
        // We expect to get an ec here when the socked is closed via a post
      }
    };

    if(socket)
    {
      assert(strand->running_in_this_thread());
      asio::async_read(*socket, asio::buffer(header.pointer(), header.size()), strand->wrap(cb));
      connected_ = true;
    }
  }

  void ReadBody(byte_array::ByteArray const &header) noexcept
  {
    auto strand = strand_.lock();
    assert(strand->running_in_this_thread());

    assert(header.size() >= sizeof(networkMagic_));
    uint64_t magic = *reinterpret_cast<const uint64_t *>(header.pointer());
    uint64_t size  = *reinterpret_cast<const uint64_t *>(header.pointer() + sizeof(uint64_t));

    if (magic != networkMagic_)
    {
      byte_array::ByteArray dummy;
      SetHeader(dummy, 0);
      dummy.Resize(16);

      fetch::logger.Error("Magic incorrect during network read:\ngot:      ",
          ToHex(header), "\nExpected: ", ToHex(byte_array::ByteArray(dummy)));
      return;
    }

    byte_array::ByteArray message;
    message.Resize(size);

    self_type self = shared_from_this();
    auto socket = socket_.lock();
    auto cb = [this, self, message, socket, strand](std::error_code ec, std::size_t len) {

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
      assert(strand->running_in_this_thread());
      asio::async_read(*socket, asio::buffer(message.pointer(), message.size()), strand->wrap(cb));
    }
  }

  static void SetHeader(byte_array::ByteArray &header, uint64_t bufSize)
  {
    header.Resize(16);

    for (std::size_t i = 0; i < 8; ++i)
    {
      header[i] = uint8_t((networkMagic_ >> i*8) & 0xff);
    }

    for (std::size_t i = 0; i < 8; ++i)
    {
      header[i+8] = uint8_t((bufSize >> i*8) & 0xff);
    }
  }

  // Always executed in a run(), in a strand
  void WriteNext(shared_self_type selfLock)
  {
    // Only one thread can get past here at a time
    // TODO: (`HUT`) : discuss with Ed: now that we are stranding might not need to lock
    if(!write_mutex_.try_lock())
    {
      return;
    }

    message_type buffer;
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
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

    auto socket = socket_.lock();

    auto cb = [this, selfLock, socket, buffer, header](std::error_code ec, std::size_t len)
    {
      assert(destructing_ == false);
      write_mutex_.unlock();

      if(ec)
      {
        fetch::logger.Error("Error writing to socket, closing.");
      }
      else
      {
        auto strandLock = strand_.lock();
        if(strandLock)
        {
          strandLock->post([this, selfLock] { WriteNext(selfLock); });
        }
      }
    };

    if(socket)
    {
      assert(strand_.lock()->running_in_this_thread());
      asio::async_write(*socket, buffers, strand_.lock()->wrap(cb));
    } else {
      fetch::logger.Error("Failed to lock socket in WriteNext!");
    }
  }

  void ConnectionFailed()
  {
    std::lock_guard< std::mutex > lock(callback_mutex_);
    if(on_connection_failed_) on_connection_failed_();
  }

  void PushMessage(message_type message)
  {
    std::lock_guard< std::mutex > lock(callback_mutex_);
    if(on_push_message_) on_push_message_(message);
  }
};

}
}
#endif
