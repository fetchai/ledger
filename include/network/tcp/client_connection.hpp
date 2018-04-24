#ifndef NETWORK_CLIENT_CONNECTION_HPP
#define NETWORK_CLIENT_CONNECTION_HPP

#include "network/tcp/client_manager.hpp"
#include "network/message.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "mutex.hpp"
#include "assert.hpp"
#include "logger.hpp"

#include "fetch_asio.hpp"
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
    LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("Server: Connection from ", socket_.remote_endpoint().address().to_string() );
  }

  ~ClientConnection() 
  {
    LOG_STACK_TRACE_POINT;
    manager_.Leave(handle_);
  }
  
  void Start() 
  {
    LOG_STACK_TRACE_POINT;
    handle_ = manager_.Join(shared_from_this());
    ReadHeader();
  }

  void Send(message_type const& msg) override
  {
    LOG_STACK_TRACE_POINT;
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
    LOG_STACK_TRACE_POINT;
    return socket_.remote_endpoint().address().to_string();    
  }

  handle_type const &handle() const {
    LOG_STACK_TRACE_POINT;
    return handle_;
  }
  
private:
  void ReadHeader() 
  {
    LOG_STACK_TRACE_POINT;

    fetch::logger.Debug("Server: Waiting for next header.");  
    auto self(shared_from_this());
    auto cb = [this, self](std::error_code ec, std::size_t len) 
      {

        if (!ec) 
        {
          fetch::logger.Debug("Server: Read header.");
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
    LOG_STACK_TRACE_POINT;

    byte_array::ByteArray message;
//    std::cout << std::hex << header_.magic << std::dec << std::endl;
//    std::cout << header_.length << std::endl;

    if( header_.content.magic != 0xFE7C80A1FE7C80A1) {
      fetch::logger.Debug("Magic incorrect - closing connection.");

      manager_.Leave(handle_);
      return;
    }
    
    message.Resize(header_.content.length);
    auto self(shared_from_this());
    auto cb = [this, self, message](std::error_code ec, std::size_t len) 
      {

        if (!ec) 
        {
          fetch::logger.Debug("Server: Read body.");
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
    LOG_STACK_TRACE_POINT;

    write_mutex_.lock();

    if(write_queue_.empty())
    {
      write_mutex_.unlock();
      return;
    }

    auto buffer = write_queue_.front();
    write_queue_.pop_front();    
    header_write_.content.magic = 0xFE7C80A1FE7C80A1;
    header_write_.content.length = buffer.size();

    write_mutex_.unlock();

    auto self = shared_from_this(); 
    auto cb = [this, buffer, self](std::error_code ec, std::size_t) 
      {
        if (!ec) 
        {
          fetch::logger.Debug("Server: Wrote message.");
          Write();
        }
        else 
        {
          manager_.Leave(handle_);
        }
      };


    std::vector< asio::const_buffer > buffers;
    buffers.push_back(asio::buffer(header_write_.bytes, 2*sizeof(uint64_t)) );
    buffers.push_back(asio::buffer(buffer.pointer(), buffer.size()) );

    asio::async_write(socket_, buffers, cb);



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
    } content;
  } header_write_, header_;
};
}
}

#endif
