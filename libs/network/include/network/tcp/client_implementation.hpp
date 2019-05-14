#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "network/management/network_manager.hpp"
#include "network/message.hpp"

#include "core/mutex.hpp"
#include "network/fetch_asio.hpp"
#include "network/management/abstract_connection.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <utility>

namespace fetch {
namespace network {

class TCPClientImplementation final : public AbstractConnection
{
public:
  using network_manager_type = NetworkManager;
  using self_type            = std::weak_ptr<AbstractConnection>;
  using shared_self_type     = std::shared_ptr<AbstractConnection>;
  using socket_type          = asio::ip::tcp::tcp::socket;
  using strand_type          = asio::io_service::strand;
  using resolver_type        = asio::ip::tcp::resolver;
  using mutex_type           = std::mutex;

  static constexpr char const *LOGGING_NAME = "TCPClientImpl";

  TCPClientImplementation(network_manager_type const &network_manager) noexcept
    : networkManager_(network_manager)
  {}

  TCPClientImplementation(TCPClientImplementation const &rhs) = delete;
  TCPClientImplementation(TCPClientImplementation &&rhs)      = delete;

  TCPClientImplementation &operator=(TCPClientImplementation const &rhs) = delete;
  TCPClientImplementation &operator=(TCPClientImplementation &&rhs) = delete;

  ~TCPClientImplementation()
  {
    LOG_STACK_TRACE_POINT;
    if (!Closed() && !posted_close_)
    {
      Close();
    }
  }

  void Connect(byte_array::ConstByteArray const &host, uint16_t port)
  {
    LOG_STACK_TRACE_POINT;
    Connect(host, byte_array::ConstByteArray(std::to_string(port)));
  }

  void Connect(byte_array::ConstByteArray const &host, byte_array::ConstByteArray const &port)
  {
    LOG_STACK_TRACE_POINT;
    self_type self = shared_from_this();

    FETCH_LOG_DEBUG(LOGGING_NAME, "Client posting connect");

    networkManager_.Post([this, self, host, port] {
      shared_self_type selfLock = self.lock();
      if (!selfLock)
      {
        return;
      }

      // We get IO objects from the network manager, they will only be strong
      // while in the post
      auto strand = networkManager_.CreateIO<strand_type>();
      if (!strand)
      {
        return;
      }
      {
        std::lock_guard<mutex_type> lock(io_creation_mutex_);
        strand_ = strand;
      }

      strand->post([this, self, host, port, strand] {
        shared_self_type selfLock = self.lock();
        if (!selfLock)
        {
          return;
        }

        std::shared_ptr<socket_type> socket = networkManager_.CreateIO<socket_type>();

        {
          std::lock_guard<mutex_type> lock(io_creation_mutex_);
          if (!posted_close_)
          {
            socket_ = socket;
          }
        }

        std::shared_ptr<resolver_type> res = networkManager_.CreateIO<resolver_type>();

        auto cb = [this, self, res, socket, strand, port](std::error_code ec,
                                                          resolver_type::iterator) {
          shared_self_type selfLock = self.lock();
          if (!selfLock)
          {
            return;
          }

          LOG_STACK_TRACE_POINT;

          FETCH_LOG_DEBUG(LOGGING_NAME, "Finished connecting.");
          if (!ec)
          {
            FETCH_LOG_DEBUG(LOGGING_NAME, "Connection established!");

            // Prevent this from throwing
            std::error_code         ec2;
            asio::ip::tcp::endpoint endpoint = (*socket).remote_endpoint(ec2);

            if (!ec2)
            {
              this->SetAddress(endpoint.address().to_string());
              this->SetPort(uint16_t(port.AsInt()));
              ReadHeader();
            }
            else
            {
              FETCH_LOG_INFO(LOGGING_NAME, "Failed to get endpoint of socket after connection");
            }
          }
          else
          {
            FETCH_LOG_INFO(LOGGING_NAME, "Client failed to connect on port ", port, ": ",
                           ec.message());
            SignalLeave();
          }
        };

        if (socket && res)
        {
          resolver_type::iterator it(res->resolve({std::string(host), std::string(port)}));

          assert(strand->running_in_this_thread());
          asio::async_connect(*socket, it, strand->wrap(cb));
        }
        else
        {
          FETCH_LOG_ERROR(LOGGING_NAME, "Failed to create valid socket");
          SignalLeave();
        }
      });  // end strand post
    });    // end NM post
  }

  bool is_alive() const override
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<mutex_type> lock(io_creation_mutex_);
    return !socket_.expired() && connected_;
  }

  void Send(message_type const &omsg) override
  {
    message_type msg = omsg.Copy();
    LOG_STACK_TRACE_POINT;
    if (!connected_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Attempting to write to socket too early. Returning.");
      return;
    }

    {
      std::lock_guard<mutex_type> lock(queue_mutex_);
      write_queue_.push_back(msg);
    }

    self_type                  self   = shared_from_this();
    std::weak_ptr<strand_type> strand = strand_;

    networkManager_.Post([this, self, strand] {
      shared_self_type selfLock   = self.lock();
      auto             strandLock = strand_.lock();
      if (!selfLock || !strandLock)
      {
        return;
      }

      strandLock->post([this, selfLock] { WriteNext(selfLock); });
    });
  }

  uint16_t Type() const override
  {

    return AbstractConnection::TYPE_OUTGOING;
  }

  void Close() override
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<mutex_type> lock(io_creation_mutex_);
    posted_close_                         = true;
    std::weak_ptr<socket_type> socketWeak = socket_;
    std::weak_ptr<strand_type> strandWeak = strand_;

    networkManager_.Post([socketWeak, strandWeak] {
      auto socket = socketWeak.lock();
      auto strand = strandWeak.lock();

      if (socket && strand)
      {
        strand->post([socket] {
          std::error_code dummy;
          socket->shutdown(asio::ip::tcp::socket::shutdown_both, dummy);
          socket->close(dummy);
        });
      }
    });
  }

  bool Closed() const override
  {
    return socket_.expired();
  }

private:
  static const uint64_t networkMagic_ = 0xFE7C80A1FE7C80A1;

  network_manager_type networkManager_;
  // IO objects should be guaranteed to have lifetime less than the
  // io_service/networkManager
  std::weak_ptr<socket_type> socket_;
  std::weak_ptr<strand_type> strand_;

  message_queue_type write_queue_;
  mutable mutex_type queue_mutex_;
  mutable mutex_type io_creation_mutex_;

  mutable mutex_type can_write_mutex_;
  bool               can_write_{true};
  bool               posted_close_ = false;

  mutable mutex_type callback_mutex_;
  std::atomic<bool>  connected_{false};

  void ReadHeader() noexcept
  {
    LOG_STACK_TRACE_POINT;
    auto strand = strand_.lock();
    if (!strand)
    {
      return;
    }
    assert(strand->running_in_this_thread());

    self_type             self   = shared_from_this();
    auto                  socket = socket_.lock();
    byte_array::ByteArray header;
    header.Resize(2 * sizeof(uint64_t));

    auto cb = [this, self, socket, header, strand](std::error_code ec, std::size_t) {
      shared_self_type selfLock = self.lock();
      if (!selfLock)
      {
        return;
      }

      if (!ec)
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "Read message header.");
        ReadBody(header);
      }
      else
      {
        // We expect to get an ec here when the socked is closed via a post
        FETCH_LOG_INFO(LOGGING_NAME, "Socket closed inside ReadHeader: ", ec.message());
        SignalLeave();
      }
    };

    if (socket)
    {
      assert(strand->running_in_this_thread());
      asio::async_read(*socket, asio::buffer(header.pointer(), header.size()), strand->wrap(cb));

      bool const previously_connected = connected_.exchange(true);

      if (!previously_connected)
      {
        SignalConnectionSuccess();
      }
    }
    else
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Socket no longer valid in ReadHeader");
      connected_ = false;
      SignalLeave();
    }
  }

  void ReadBody(byte_array::ByteArray const &header) noexcept
  {
    LOG_STACK_TRACE_POINT;
    auto strand = strand_.lock();
    assert(strand->running_in_this_thread());

    assert(header.size() >= sizeof(networkMagic_));
    uint64_t magic = *reinterpret_cast<const uint64_t *>(header.pointer());
    uint64_t size  = *reinterpret_cast<const uint64_t *>(header.pointer() + sizeof(uint64_t));

    /*FETCH_LOG_INFO(LOGGING_NAME, "Receive size: ", size);*/

    if (magic != networkMagic_)
    {
      byte_array::ByteArray dummy;
      SetHeader(dummy, 0);
      dummy.Resize(16);

      FETCH_LOG_ERROR(LOGGING_NAME,
                      "Magic incorrect during network read:\ngot:      ", ToHex(header),
                      "\nExpected: ", ToHex(byte_array::ByteArray(dummy)));
      return;
    }

    byte_array::ByteArray message;
    message.Resize(size);

    self_type self   = shared_from_this();
    auto      socket = socket_.lock();
    auto      cb     = [this, self, message, socket, strand](std::error_code ec, std::size_t len) {
      FETCH_UNUSED(len);

      shared_self_type selfLock = self.lock();
      if (!selfLock)
      {
        return;
      }

      if (!ec)
      {
        SignalMessage(message);
        ReadHeader();
      }
      else
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Reading body failed, dying: ", ec, ec.message());
        SignalLeave();
      }
    };

    if (socket)
    {
      assert(strand->running_in_this_thread());
      asio::async_read(*socket, asio::buffer(message.pointer(), message.size()), strand->wrap(cb));
    }
    else
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Invalid socket when attempting to read body");
      SignalLeave();
    }
  }

  static void SetHeader(byte_array::ByteArray &header, uint64_t bufSize)
  {
    header.Resize(16);

    for (std::size_t i = 0; i < 8; ++i)
    {
      header[i] = uint8_t((networkMagic_ >> i * 8) & 0xff);
    }

    for (std::size_t i = 0; i < 8; ++i)
    {
      header[i + 8] = uint8_t((bufSize >> i * 8) & 0xff);
    }
  }

  // Always executed in a run(), in a strand
  void WriteNext(shared_self_type selfLock)
  {
    // Only one thread can get past here at a time. Effectively a try_lock
    // except that we can't unlock a mutex in the callback (undefined behaviour)
    {
      std::lock_guard<mutex_type> lock(can_write_mutex_);
      if (can_write_)
      {
        can_write_ = false;
      }
      else
      {
        return;
      }
    }

    message_type buffer;
    {
      std::lock_guard<mutex_type> lock(queue_mutex_);
      if (write_queue_.empty())
      {
        std::lock_guard<mutex_type> lock(can_write_mutex_);
        can_write_ = true;
        return;
      }
      buffer = write_queue_.front();
      write_queue_.pop_front();
    }

    byte_array::ByteArray header;
    SetHeader(header, buffer.size());

    std::vector<asio::const_buffer> buffers{asio::buffer(header.pointer(), header.size()),
                                            asio::buffer(buffer.pointer(), buffer.size())};

    auto socket = socket_.lock();

    auto cb = [this, selfLock, socket, buffer, header](std::error_code ec, std::size_t len) {
      FETCH_UNUSED(len);

      {
        std::lock_guard<mutex_type> lock(can_write_mutex_);
        can_write_ = true;
      }

      if (ec)
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Error writing to socket, closing.");
        SignalLeave();
      }
      else
      {
        // TODO(issue 16): this strand should be unnecessary
        auto strandLock = strand_.lock();
        if (strandLock)
        {
          WriteNext(selfLock);
        }
      }
    };

    auto strand = strand_.lock();

    if (socket && strand)
    {
      assert(strand->running_in_this_thread());
      asio::async_write(*socket, buffers, strand->wrap(cb));
    }
    else
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to lock socket in WriteNext!");
      SignalLeave();
    }
  }
};

}  // namespace network
}  // namespace fetch
