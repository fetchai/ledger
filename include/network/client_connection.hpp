#ifndef NETWORK_CLIENT_CONNECTION_HPP
#define NETWORK_CLIENT_CONNECTION_HPP

#include "network/client_manager.hpp"
#include "network/message.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "mutex.hpp"
#include "assert.hpp"

#include <asio.hpp>

namespace fetch {
namespace network {

class ClientConnection : public AbstractClientConnection,
                         public std::enable_shared_from_this<ClientConnection> {
 public:
  typedef typename AbstractClientConnection::shared_type connection_type;

  typedef ClientManager::handle_type handle_type;

  ClientConnection(asio::ip::tcp::tcp::socket socket, ClientManager& manager)
    : socket_(std::move(socket)), manager_(manager), write_mutex_(__LINE__, __FILE__) {}

  ~ClientConnection() {
    manager_.Leave(handle_);
  }
  
  void Start() {
    handle_ = manager_.Join(shared_from_this());
    ReadHeader();
  }

  void Send(message_type const& msg) {
    write_mutex_.lock();
    bool write_in_progress = !write_queue_.empty();  
    write_queue_.push_back(msg);
    write_mutex_.unlock();
    
    if (!write_in_progress) {
      Write();
    }
  }

 private:
  void ReadHeader() {

    auto self(shared_from_this());
    auto cb = [this, self](std::error_code ec, std::size_t) {
      if (!ec) {


        if( header_.length >= 10000 ) {
          std::cout << "Contents: ";
          for(std::size_t i=0; i< sizeof(uint64_t); ++i)
            std::cout << char(header_.bytes[i]);
          std::cout << std::endl;
          std::cout << "Contents: ";
          for(std::size_t i=0; i< sizeof(uint64_t); ++i)
            std::cout << uint64_t(header_.bytes[i]) << " ";
          std::cout << std::endl;          
        }
        detailed_assert(header_.length < 10000);        
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
    write_mutex_.lock();    
    buffer << write_queue_.front();
    write_mutex_.unlock();

    auto cb = [this](std::error_code ec, std::size_t) {
      if (!ec) {
        write_mutex_.lock();
        write_queue_.pop_front();
        bool write_more = !write_queue_.empty();
        write_mutex_.unlock();  
        if (write_more) {
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
  fetch::mutex::Mutex write_mutex_;
  handle_type handle_;

  union {
    char bytes[sizeof(uint64_t)];
    uint64_t length;
  } header_;
};
};
};

#endif
