#ifndef NETWORK_NETWORK_CLIENT_HPP
#define NETWORK_NETWORK_CLIENT_HPP

#include "network/message.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/referenced_byte_array.hpp"

#include <asio.hpp>
#include <memory>
#include <mutex>

namespace fetch {
namespace network {
class NetworkClient {
 public:
  typedef byte_array::ReferencedByteArray byte_array_type;

  NetworkClient(byte_array_type const& host, byte_array_type const& port)
      : socket_(io_service_) {
    Connect(host, port);
  }

  NetworkClient(byte_array_type const& host, uint16_t const& port)
      : socket_(io_service_) {
    Connect(host, port);
  }

  ~NetworkClient() {
    Stop();

    socket_.close();
  }

  void Send(message_type const& msg) {
    auto cb = [this, msg]() {
      bool write_in_progress = !write_queue_.empty();
      write_queue_.push_back(msg);
      if (!write_in_progress) {
        Write();
      }
    };

    io_service_.post(cb);
  }

  void Start() {
    if (thread_ == nullptr) {
      thread_ = new std::thread([this]() { io_service_.run(); });
    }
  }

  void Stop() {
    if (thread_ != nullptr) {
      io_service_.stop();
      thread_->join();
      delete thread_;
      thread_ = nullptr;
    }
  }

  virtual void PushMessage(message_type const& value) = 0;

 private:
  void Connect(std::string const& host, std::string const& port) {
    asio::ip::tcp::resolver resolver(io_service_);
    Connect(resolver.resolve({host, port}));
  }

  void Connect(std::string const& host, uint16_t const& port) {
    std::stringstream p;
    p << port;

    asio::ip::tcp::resolver resolver(io_service_);
    Connect(resolver.resolve({host, p.str()}));
  }

  void Connect(asio::ip::tcp::tcp::resolver::iterator endpoint_iterator) {
    auto cb = [this](std::error_code ec,
                     asio::ip::tcp::tcp::resolver::iterator) {
      if (!ec) {
        ReadHeader();
      }
    };
    asio::async_connect(socket_, endpoint_iterator, cb);
  }

  void ReadHeader() {
    auto cb = [this](std::error_code ec, std::size_t) {
      if (!ec) {
        // TODO: Take care of endianness
        ReadBody();
      } else {
        socket_.close();
      }
    };

    asio::async_read(socket_, asio::buffer(header_.bytes, sizeof(uint64_t)),
                     cb);
  }

  void ReadBody() {
    message_type message;
    message.Resize(header_.length);

    auto cb = [this, message](std::error_code ec, std::size_t) {
      if (!ec) {
        PushMessage(message);
        ReadHeader();
      } else {

        socket_.close();
      }
    };

    asio::async_read(socket_, asio::buffer(message.pointer(), message.size()),
                     cb);
  }

  void Write() {
    serializers::Byte_ArrayBuffer buffer;
    write_mutex_.lock();
    bool should_write = !write_queue_.empty();
    if(should_write) {
      buffer << write_queue_.front();
      write_queue_.pop_front();
    }
    write_mutex_.unlock();
    
    auto cb = [this](std::error_code ec, std::size_t) {
      if (!ec) {
        write_mutex_.lock();
        bool write_more = !write_queue_.empty();    
        write_mutex_.unlock();          
        if (!write_more) {
          Write();
        }
      } else {

        socket_.close();
      }
    };

    if(should_write) {
      asio::async_write(
                        socket_, asio::buffer(buffer.data().pointer(), buffer.data().size()),
                        cb);
    }
  }

 private:
  union {
    char bytes[sizeof(uint64_t)];
    uint64_t length;
  } header_;

  std::thread* thread_ = nullptr;
  asio::io_service io_service_;
  asio::ip::tcp::tcp::socket socket_;

  message_queue_type write_queue_;
  fetch::mutex::Mutex write_mutex_;

};
};
};

#endif
