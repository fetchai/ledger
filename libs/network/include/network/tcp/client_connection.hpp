#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

  using handle_type = typename AbstractConnection::connection_handle_type;

  ClientConnection(std::weak_ptr<asio::ip::tcp::tcp::socket> socket,
                   std::weak_ptr<ClientManager>              manager)
    : socket_(std::move(socket)), manager_(std::move(manager)), write_mutex_(__LINE__, __FILE__)
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

        FETCH_LOG_DEBUG(LOGGING_NAME,"Server: Connection from ",
                            socket_ptr->remote_endpoint().address().to_string());
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME,"Server: Failed to get endpoint for socket!");
      }
    }
  }

  ~ClientConnection()
  {
    LOG_STACK_TRACE_POINT;
    auto ptr = manager_.lock();
    if (!ptr) return;

    ptr->Leave(this->handle());
  }

  void Start()
  {
    LOG_STACK_TRACE_POINT;
    auto ptr = manager_.lock();
    if (!ptr) return;

    ptr->Join(shared_from_this());
    ReadHeader();
  }

  void Send(message_type const &msg) override
  {
    LOG_STACK_TRACE_POINT;

    if (shutting_down_)
      return;

    write_mutex_.lock();
    bool write_in_progress = !write_queue_.empty();
    write_queue_.push_back(msg);
    write_mutex_.unlock();

    FETCH_LOG_INFO(LOGGING_NAME, "Sending Message");

    if (!write_in_progress)
    {
      Write();
    }
  }

  uint16_t Type() const override { return AbstractConnection::TYPE_INCOMING; }

  void Close() override {
    shutting_down_ = true;
    auto socket = socket_.lock();
    if (socket)
    {
      std::error_code ec;
      socket->close(ec);
    }
  }

  bool Closed() override {
    return static_cast<bool>(socket_.lock());
  }

  bool is_alive() const override { return !static_cast<bool>(socket_.lock()); }

private:
  void ReadHeader()
  {
    if (shutting_down_)
      return;

    LOG_STACK_TRACE_POINT;
    auto socket_ptr = socket_.lock();
    if (!socket_ptr) return;

    FETCH_LOG_DEBUG(LOGGING_NAME,"Server: Waiting for next header.");
    auto self(shared_from_this());
    auto cb = [this, socket_ptr, self](std::error_code ec, std::size_t len) {
      auto ptr = manager_.lock();
      if (!ptr)
      {
        return;
      }

      if (!ec)
      {
        FETCH_LOG_DEBUG(LOGGING_NAME,"Server: Read header.");
        ReadBody();
      }
      else
      {
        ptr->Leave(this->handle());
      }
    };

    asio::async_read(*socket_ptr, asio::buffer(header_.bytes, 2 * sizeof(uint64_t)), cb);
  }

  void ReadBody()
  {
    LOG_STACK_TRACE_POINT;

    if (shutting_down_)
      return;

    auto socket_ptr = socket_.lock();
    if (!socket_ptr) return;

    byte_array::ByteArray message;

    if (header_.content.magic != networkMagic_)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME,"Magic incorrect - closing connection.");
      auto ptr = manager_.lock();
      if (!ptr) return;
      ptr->Leave(this->handle());
      return;
    }

    message.Resize(header_.content.length);
    auto self(shared_from_this());
    auto cb = [this, socket_ptr, self, message](std::error_code ec, std::size_t len) {
      auto ptr = manager_.lock();
      if (!ptr)
      {
        return;
      }

      if (!ec)
      {
        FETCH_LOG_INFO(LOGGING_NAME,"Server: Recv message");

        ptr->PushRequest(this->handle(), message);
        ReadHeader();
      }
      else
      {
        ptr->Leave(this->handle());
      }
    };

    asio::async_read(*socket_ptr, asio::buffer(message.pointer(), message.size()), cb);
  }

  void SetHeader(byte_array::ByteArray &header, uint64_t bufSize)
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

  void Write()
  {
    if (shutting_down_)
      return;

    LOG_STACK_TRACE_POINT;
    auto socket_ptr = socket_.lock();
    if (!socket_ptr) return;

    write_mutex_.lock();

    if (write_queue_.empty())
    {
      write_mutex_.unlock();
      return;
    }

    auto buffer = write_queue_.front();

    byte_array::ByteArray header;
    SetHeader(header, buffer.size());
    write_queue_.pop_front();
    write_mutex_.unlock();

    auto self = shared_from_this();
    auto cb   = [this, buffer, socket_ptr, header, self](std::error_code ec, std::size_t) {
      auto ptr = manager_.lock();
      if (!ptr) return;

      if (!ec)
      {
        FETCH_LOG_DEBUG(LOGGING_NAME,"Server: Wrote message.");
        Write();
      }
      else
      {
        ptr->Leave(this->handle());
      }
    };

    std::vector<asio::const_buffer> buffers{asio::buffer(header.pointer(), header.size()),
                                            asio::buffer(buffer.pointer(), buffer.size())};

    asio::async_write(*socket_ptr, buffers, cb);
  }

  std::atomic<bool>                         shutting_down_{false};
  std::weak_ptr<asio::ip::tcp::tcp::socket> socket_;
  std::weak_ptr<ClientManager>              manager_;
  message_queue_type                        write_queue_;
  fetch::mutex::Mutex                       write_mutex_;
  std::string                               address_;

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
};
}  // namespace network
}  // namespace fetch
