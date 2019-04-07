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

#include "core/assert.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "network/management/client_manager.hpp"
#include "network/management/network_manager.hpp"
#include "network/message.hpp"

#include "network/fetch_asio.hpp"
#include <atomic>
#include <utility>

namespace fetch {
namespace network {

/*
 * ClientConnections are spawned by the server when external clients connect.
 * The class will generically push data to its manager, and also allow pushing
 * data to the connected client.
 */
class ClientConnection : public AbstractConnection
{
public:
  static constexpr char const *LOGGING_NAME = "ClientConnection";

  using connection_type = typename AbstractConnection::shared_type;

  using handle_type      = typename AbstractConnection::connection_handle_type;
  using Strand           = asio::io_service::strand;
  using StrongStrand     = std::shared_ptr<asio::io_service::strand>;
  using Socket           = asio::ip::tcp::tcp::socket;
  using shared_self_type = std::shared_ptr<AbstractConnection>;
  using mutex_type       = std::mutex;

  ClientConnection(std::weak_ptr<asio::ip::tcp::tcp::socket> socket,
                   std::weak_ptr<ClientManager> manager, NetworkManager network_manager)
    : socket_(std::move(socket))
    , manager_(std::move(manager))
    , network_manager_(network_manager)
  {
    LOG_STACK_TRACE_POINT;
    auto socket_ptr = socket_.lock();
    if (socket_ptr)
    {

      // Prevent this from throwing
      std::error_code         ec;
      asio::ip::tcp::endpoint endpoint = (*socket_ptr).remote_endpoint(ec);

      if (!ec)
      {
        this->SetAddress(endpoint.address().to_string());

        FETCH_LOG_DEBUG(LOGGING_NAME, "Server: Connection from ",
                        socket_ptr->remote_endpoint().address().to_string());
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Server: Failed to get endpoint for socket!");
      }
    }
  }

  ~ClientConnection()
  {
    LOG_STACK_TRACE_POINT;
    auto ptr = manager_.lock();
    if (ptr)
    {
      ptr->Leave(this->handle());
    }
  }

  void Start()
  {
    LOG_STACK_TRACE_POINT;
    auto ptr = manager_.lock();
    if (ptr)
    {
      ptr->Join(shared_from_this());
    }

    auto strong_strand = network_manager_.CreateIO<Strand>();
    if (!strong_strand)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Failed to create strand. Will not read header");
      return;
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Created strand");

    strand_ = strong_strand;

    ReadHeader(strong_strand);
  }

  void Send(message_type const &msg) override
  {
    if (shutting_down_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Attempting to write to socket while it's shut down.");
      return;
    }

    {
      FETCH_LOCK(queue_mutex_);
      if (write_queue_.empty())
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "Sending Message");
        write_queue_.push_back(msg);
      }
    }

    std::weak_ptr<AbstractConnection> self   = shared_from_this();
    std::weak_ptr<Strand>             strand = strand_;

    network_manager_.Post([this, self, strand] {
      shared_self_type selfLock   = self.lock();
      auto             strandLock = strand_.lock();
      if (!selfLock || !strandLock)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to lock. Strand: ", bool(selfLock),
                       " socket: ", bool(strandLock));
        return;
      }

      strandLock->post([this, selfLock] { WriteNext(selfLock); });
    });
  }

  uint16_t Type() const override
  {
    return AbstractConnection::TYPE_INCOMING;
  }

  void Close() override
  {
    shutting_down_ = true;
    auto socket    = socket_.lock();
    if (socket)
    {
      std::error_code ec;
      socket->close(ec);
    }
  }

  bool Closed() const override
  {
    return !static_cast<bool>(socket_.lock());
  }

  bool is_alive() const override
  {
    return static_cast<bool>(socket_.lock());

    // This does not appear to tell the whole story. Consider a full
    // state DISCONNECTED->CONNECTED->DROPPED
  }

private:
  std::atomic<bool>                         shutting_down_{false};
  std::weak_ptr<asio::ip::tcp::tcp::socket> socket_;
  std::weak_ptr<ClientManager>              manager_;

  std::string address_;

  NetworkManager network_manager_;
  // bool                  posted_close_ = false;
  std::weak_ptr<Strand> strand_;

  message_queue_type write_queue_;
  mutable mutex_type can_write_mutex_;
  bool               can_write_{true};
  mutable mutex_type queue_mutex_;

  // TODO(issue 17): put this in shared class
  static const uint64_t networkMagic_ = 0xFE7C80A1FE7C80A1;

  // TODO(issue 17): fix this to be self-contained
  union
  {
    char bytes[2 * sizeof(uint64_t)];
    struct
    {
      uint64_t magic;
      uint64_t length;
    } content;
  } header_;

  void ReadHeader(StrongStrand strong_strand)
  {
    if (shutting_down_)
    {
      return;
    }

    LOG_STACK_TRACE_POINT;
    auto socket_ptr = socket_.lock();
    if (!socket_ptr)
    {
      return;
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Server: Waiting for next header.");
    auto self(shared_from_this());
    auto cb = [this, socket_ptr, self, strong_strand](std::error_code ec, std::size_t len) {
      FETCH_UNUSED(len);

      auto ptr = manager_.lock();
      if (!ptr)
      {
        return;
      }

      if (!ec)
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "Server: Read header.");
        ReadBody(strong_strand);
      }
      else
      {
        ptr->Leave(this->handle());
      }
    };

    asio::async_read(*socket_ptr, asio::buffer(header_.bytes, 2 * sizeof(uint64_t)), cb);
  }

  void ReadBody(StrongStrand strong_strand)
  {
    LOG_STACK_TRACE_POINT;

    if (shutting_down_)
    {
      return;
    }

    auto socket_ptr = socket_.lock();
    if (!socket_ptr)
    {
      return;
    }

    byte_array::ByteArray message;

    if (header_.content.magic != networkMagic_)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Magic incorrect - closing connection.");
      auto ptr = manager_.lock();
      if (ptr)
      {
        ptr->Leave(handle());
      }
      return;
    }

    message.Resize(header_.content.length);
    auto self(shared_from_this());
    auto cb = [this, socket_ptr, self, message, strong_strand](std::error_code ec,
                                                               std::size_t     len) {
      FETCH_UNUSED(len);

      auto ptr = manager_.lock();
      if (!ptr)
      {
        return;
      }

      if (!ec)
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "Server: Recv message");

        ptr->PushRequest(this->handle(), message);
        ReadHeader(strong_strand);
      }
      else
      {
        ptr->Leave(this->handle());
      }
    };

    asio::async_read(*socket_ptr, asio::buffer(message.pointer(), message.size()), cb);
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
      FETCH_LOCK(can_write_mutex_);
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
      FETCH_LOCK(queue_mutex_);
      if (write_queue_.empty())
      {
        FETCH_LOCK(can_write_mutex_);
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
        FETCH_LOCK(can_write_mutex_);
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
