#ifndef NETWORK_CLIENT_CONNECTION_HPP
#define NETWORK_CLIENT_CONNECTION_HPP

#include "network/client_manager.hpp"
#include "network/message.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/referenced_byte_array.hpp"

#include <asio.hpp>

namespace fetch {
namespace network {

class ClientConnection : public AbstractClientConnection,
                         public std::enable_shared_from_this<ClientConnection> {
 public:
  typedef typename AbstractClientConnection::shared_type connection_type;

  typedef ClientManager::handle_type handle_type;

  ClientConnection(asio::ip::tcp::tcp::socket socket, ClientManager& manager)
      : socket_(std::move(socket)), manager_(manager) {}

  void Start() {
    handle_ = manager_.Join(shared_from_this());
    ReadHeader();
  }

  void Send(message_type const& msg) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    bool write_in_progress = !write_queue_.empty();  // TODO: add mutex
    write_queue_.push_back(msg);
    if (!write_in_progress) {
      Write();
    }
  }

 private:
  void ReadHeader() {
    auto self(shared_from_this());
    auto cb = [this, self](std::error_code ec, std::size_t) {
      if (!ec) {
        // TODO: Take care of endianness
        ReadBody();
      } else {
        manager_.Leave(handle_);
      }
    };

    asio::async_read(socket_, asio::buffer(header_.bytes, sizeof(uint64_t)),
                     cb);
  }

  void ReadBody() {
    message_type message;
    message.Resize(header_.length);
    auto self(shared_from_this());
    auto cb = [this, self, message](std::error_code ec, std::size_t) {
      if (!ec) {
        manager_.PushRequest(handle_, message);
        ReadHeader();
      } else {
        manager_.Leave(handle_);
      }
    };

    asio::async_read(socket_, asio::buffer(message.pointer(), message.size()),
                     cb);
  }

  void Write() {
    serializers::Byte_ArrayBuffer buffer;
    buffer << write_queue_.front();

    auto cb = [this](std::error_code ec, std::size_t) {
      if (!ec) {
        write_queue_.pop_front();
        if (!write_queue_.empty()) {
          Write();
        }
      } else {
        manager_.Leave(handle_);
      }
    };

    asio::async_write(
        socket_, asio::buffer(buffer.data().pointer(), buffer.data().size()),
        cb);
  }

  asio::ip::tcp::tcp::socket socket_;
  ClientManager& manager_;
  message_queue_type write_queue_;
  std::mutex write_mutex_;
  handle_type handle_;

  union {
    char bytes[sizeof(uint64_t)];
    uint64_t length;
  } header_;
};
};
};

#endif
