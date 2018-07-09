#ifndef NETWORK_CLIENT_CONNECTION_HPP
#define NETWORK_CLIENT_CONNECTION_HPP

#include "core/assert.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/message.hpp"
#include "network/tcp/client_manager.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/referenced_byte_array.hpp"

#include "network/fetch_asio.hpp"
#include<atomic>
namespace fetch {
namespace network {

/*
 * ClientConnections are spawned by the server when external clients connect. The 
 * class will generically push data to its manager, and also allow pushing data to the 
 * connected client.
 */

class ClientConnection : public AbstractConnection {
 public:
  typedef typename AbstractConnection::shared_type connection_type;

  typedef typename AbstractConnection::connection_handle_type handle_type;

  ClientConnection(std::weak_ptr< asio::ip::tcp::tcp::socket > socket , std::weak_ptr< ClientManager > manager)
      : socket_(socket),
        manager_(manager),
        write_mutex_(__LINE__, __FILE__) {
    LOG_STACK_TRACE_POINT;
    auto socket_ptr = socket_.lock();
    if(socket_ptr) {
      this->SetAddress(socket_ptr->remote_endpoint().address().to_string());
      fetch::logger.Debug("Server: Connection from ",
        socket_ptr->remote_endpoint().address().to_string());
    }
    
  }

  ~ClientConnection() {
    LOG_STACK_TRACE_POINT;
    auto ptr = manager_.lock();
    if(!ptr) return;

    ptr->Leave(this->handle());
  }

  void Start() {
    LOG_STACK_TRACE_POINT;
    auto ptr = manager_.lock();
    if(!ptr) return;
    
    ptr->Join(shared_from_this());
    ReadHeader();
  }

  void Send(message_type const& msg) override {
    LOG_STACK_TRACE_POINT;
    write_mutex_.lock();
    bool write_in_progress = !write_queue_.empty();
    write_queue_.push_back(msg);
    write_mutex_.unlock();

    if (!write_in_progress) {
      Write();
    }
  }


  uint16_t Type() const override 
  {
    return AbstractConnection::TYPE_INCOMING;
  }


  void Close() override 
  {
    TODO_FAIL("not implemented");    
  }
  
  bool Closed() override 
  {
    TODO_FAIL("not implemented");    
  }
  
  bool is_alive() const override
  {
    TODO_FAIL("not implemented");    
  }
  
  
 private:
  void ReadHeader() {
    LOG_STACK_TRACE_POINT;
    auto socket_ptr = socket_.lock();
    if(!socket_ptr) return;

    fetch::logger.Debug("Server: Waiting for next header.");
    auto self(shared_from_this());
    auto cb = [this,socket_ptr, self](std::error_code ec, std::size_t len) {
      auto ptr = manager_.lock();
      if(!ptr) {
        return;
      }
      
      if (!ec) {
        fetch::logger.Debug("Server: Read header.");
        ReadBody();
      } else {
        
        ptr->Leave(this->handle());
      }
    };
    
    asio::async_read(*socket_ptr, asio::buffer(header_.bytes, 2 * sizeof(uint64_t)),
                     cb);
  }

  void ReadBody() {
    LOG_STACK_TRACE_POINT;
    auto socket_ptr = socket_.lock();
    if(!socket_ptr) return;
    
    byte_array::ByteArray message;

    if (header_.content.magic != networkMagic) {
      fetch::logger.Debug("Magic incorrect - closing connection.");
      auto ptr = manager_.lock();
      if(!ptr) return;
      ptr->Leave(this->handle());
      return;
    }

    message.Resize(header_.content.length);
    auto self(shared_from_this());
    auto cb = [this, socket_ptr, self, message](std::error_code ec, std::size_t len) {
      auto ptr = manager_.lock();
      if(!ptr)  {
        return;
      }
      
      
      if (!ec) {
        fetch::logger.Debug("Server: Read body.");
        ptr->PushRequest(this->handle(), message);
        ReadHeader();
      } else {

        ptr->Leave(this->handle());

      }
    };

    asio::async_read(*socket_ptr, asio::buffer(message.pointer(), message.size()),
                     cb);
  }

  void SetHeader(byte_array::ByteArray &header, uint64_t bufSize)
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

  void Write() {
    LOG_STACK_TRACE_POINT;
    auto socket_ptr = socket_.lock();
    if(!socket_ptr) return;

    write_mutex_.lock();

    if (write_queue_.empty()) {
      write_mutex_.unlock();
      return;
    }

    auto buffer = write_queue_.front();
    byte_array::ByteArray header;
    SetHeader(header, buffer.size());
    write_queue_.pop_front();
    write_mutex_.unlock();

    auto self = shared_from_this();
    auto cb = [this, buffer,socket_ptr, header, self](std::error_code ec, std::size_t) {
      auto ptr = manager_.lock();
      if(!ptr) return;

      if (!ec) {
        fetch::logger.Debug("Server: Wrote message.");
        Write();
      } else {
        ptr->Leave(this->handle());
      }
    };

    std::vector<asio::const_buffer> buffers{
      asio::buffer(header.pointer(), header.size()),
      asio::buffer(buffer.pointer(), buffer.size())};

    asio::async_write(*socket_ptr, buffers, cb);
  }

  std::weak_ptr< asio::ip::tcp::tcp::socket > socket_;
  std::weak_ptr< ClientManager > manager_;
  message_queue_type write_queue_;
  fetch::mutex::Mutex write_mutex_;
  std::string address_;
  const uint64_t networkMagic = 0xFE7C80A1FE7C80A1; // TODO: (`HUT`) : put this in shared class

  // TODO: (`HUT`) : fix this to be self-contained
  union {
    char bytes[2 * sizeof(uint64_t)];
    struct {
      uint64_t magic;
      uint64_t length;
    } content;
  } header_;
};
}
}

#endif
