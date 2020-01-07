#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "core/mutex.hpp"
#include "core/serializers/main_serializer.hpp"
#include "logging/logging.hpp"
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

  using ConnectionType = typename AbstractConnection::SharedType;

  using HandleType     = typename AbstractConnection::ConnectionHandleType;
  using Strand         = asio::io_service::strand;
  using StrongStrand   = std::shared_ptr<asio::io_service::strand>;
  using Socket         = asio::ip::tcp::tcp::socket;
  using SharedSelfType = std::shared_ptr<AbstractConnection>;
  using MutexType      = std::mutex;

  ClientConnection(std::weak_ptr<asio::ip::tcp::tcp::socket> socket,
                   std::weak_ptr<ClientManager> manager, NetworkManager network_manager)
    : socket_(std::move(socket))
    , manager_(std::move(manager))
    , network_manager_(network_manager)  // NOLINT
  {
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

  ~ClientConnection() override
  {
    auto ptr = manager_.lock();
    if (!ptr)
    {
      return;
    }

    ptr->Leave(this->handle());
  }

  void Start()
  {
    auto ptr = manager_.lock();
    if (!ptr)
    {
      return;
    }

    ptr->Join(shared_from_this());

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

  void Send(MessageBuffer const &msg, Callback const &success = nullptr,
            Callback const &fail = nullptr) override
  {
    if (shutting_down_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Attempting to write to socket while it's shut down.");
      return;
    }

    {
      FETCH_LOCK(queue_mutex_);
      // write_queue_.emplace_back(msg);
      write_queue_.push_back({msg, success, fail});
    }

    std::weak_ptr<AbstractConnection> self   = shared_from_this();
    std::weak_ptr<Strand>             strand = strand_;

    network_manager_.Post([this, self, strand] {
      SharedSelfType selfLock   = self.lock();
      auto           strandLock = strand_.lock();
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
    DeactivateSelfManage();

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

  MessageQueueType  write_queue_;
  mutable MutexType can_write_mutex_;
  bool              can_write_{true};
  mutable MutexType queue_mutex_;

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
  } header_{};

  void ReadHeader(StrongStrand const &strong_strand)
  {
    if (shutting_down_)
    {
      return;
    }

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

  void ReadBody(StrongStrand const &strong_strand)
  {
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
      if (!ptr)
      {
        return;
      }
      ptr->Leave(this->handle());
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
  void WriteNext(SharedSelfType const &selfLock)
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

    MessageType message;
    {
      FETCH_LOCK(queue_mutex_);
      if (write_queue_.empty())
      {
        FETCH_LOCK(can_write_mutex_);
        can_write_ = true;
        return;
      }
      message = write_queue_.front();
      write_queue_.pop_front();
    }

    byte_array::ByteArray header;
    SetHeader(header, message.buffer.size());

    std::vector<asio::const_buffer> buffers{
        asio::buffer(header.pointer(), header.size()),
        asio::buffer(message.buffer.pointer(), message.buffer.size())};

    auto socket = socket_.lock();

    auto cb = [this, selfLock, socket, message, header](std::error_code ec, std::size_t len) {
      FETCH_UNUSED(len);

      {
        FETCH_LOCK(can_write_mutex_);
        can_write_ = true;
      }

      if (ec)
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Error writing to socket, closing.");
        SignalLeave();

        if (message.failure)
        {
          message.failure();
        }
      }
      else
      {
        // TODO(issue 16): this strand should be unnecessary
        if (message.success)
        {
          message.success();
        }

        auto strandLock = strand_.lock();
        if (strandLock)
        {
          if (message.success)
          {
            message.success();
          }
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
      if (!shutting_down_)
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Failed to lock socket in WriteNext!");
      }

      SignalLeave();
      if (message.failure)
      {
        message.failure();
      }
    }
  }
};
}  // namespace network
}  // namespace fetch
