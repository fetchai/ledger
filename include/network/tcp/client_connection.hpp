#ifndef NETWORK_CLIENT_CONNECTION_HPP
#define NETWORK_CLIENT_CONNECTION_HPP

#include "network/tcp/client_manager.hpp"
#include "network/message.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "mutex.hpp"
#include "assert.hpp"
#include "logger.hpp"

#include <asio.hpp>

namespace fetch 
{
namespace network 
{

class ClientConnection : public AbstractClientConnection,
                         public std::enable_shared_from_this<ClientConnection> 
{
public:
  typedef typename AbstractClientConnection::shared_type connection_type;
  
  typedef ClientManager::handle_type handle_type;

  ClientConnection(asio::ip::tcp::tcp::socket socket, ClientManager& manager)
    : socket_(std::move(socket)), manager_(manager), write_mutex_(__LINE__, __FILE__)
  {
    fetch::logger.Debug("Connection from ", socket_.remote_endpoint().address().to_string() );
  }

  ~ClientConnection() 
  {
    manager_.Leave(handle_);
  }
  
  void Start() 
  {
    handle_ = manager_.Join(shared_from_this());
    ReadHeader();
  }

  void Send(message_type const& msg) override
  {
    write_mutex_.lock();
    bool write_in_progress = !write_queue_.empty();  
    write_queue_.push_back(msg);
    write_mutex_.unlock();
    
    if (!write_in_progress) 
    {
      Write();
    }
  }

  std::string Address() override
  {
    return socket_.remote_endpoint().address().to_string();    
  }
  
private:
  void ReadHeader() 
  {
    fetch::logger.Debug("Waiting for next header.");  
    auto self(shared_from_this());
    auto cb = [this, self](std::error_code ec, std::size_t) 
      {
        if (!ec) 
        {
          fetch::logger.Debug("Read header.");
          ReadBody();
        } else 
        {
          manager_.Leave(handle_);
        }
      };

    asio::async_read(socket_, asio::buffer(header_.bytes, 2*sizeof(uint64_t)),
      cb);
  }

  void ReadBody() 
  {
    byte_array::ByteArray message;
    std::cout << std::hex << header_.magic << std::dec << std::endl;
    std::cout << header_.length << std::endl;

    if( header_.magic != 0xFE7C80A1FE7C80A1) {
      fetch::logger.Debug("Magic incorrect - closing connection.");
      manager_.Leave(handle_);
      return;
    }
    
    message.Resize(header_.length);
    auto self(shared_from_this());
    auto cb = [this, self, message](std::error_code ec, std::size_t) 
      {
        if (!ec) 
        {
          manager_.PushRequest(handle_, message);
          ReadHeader();
        } else 
        {
          manager_.Leave(handle_);
        }
      };

    asio::async_read(socket_, asio::buffer(message.pointer(), message.size()),
      cb);
  }

  void Write() 
  {
    serializers::ByteArrayBuffer buffer;
    write_mutex_.lock();
    buffer << 0xFE7C80A1FE7C80A1;
    buffer << write_queue_.front();
    write_mutex_.unlock();

    auto self = shared_from_this(); 
    auto cb = [this, buffer, self](std::error_code ec, std::size_t) 
      {
        if (!ec) 
        {
          write_mutex_.lock();
          write_queue_.pop_front();
          bool write_more = !write_queue_.empty();
          write_mutex_.unlock();  
          if (write_more) 
          {
            Write();
          }
        }
        else 
        {
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
  std::string address_;
  
  union 
  {
    char bytes[2*sizeof(uint64_t)];
    struct {
      uint64_t magic;      
      uint64_t length;
    };
  } header_;
};
};
};

#endif
