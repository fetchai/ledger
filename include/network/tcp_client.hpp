#ifndef NETWORK_TCP_CLIENT_HPP
#define NETWORK_TCP_CLIENT_HPP

#include "network/thread_manager.hpp"
#include "network/message.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/byte_array_buffer.hpp"
#include"logger.hpp"

#include "mutex.hpp"
#include <asio.hpp>
#include <memory>
#include <mutex>
#include <atomic>

namespace fetch 
{
namespace network 
{
class TCPClient 
{
public:
  typedef ThreadManager thread_manager_type;  
  typedef thread_manager_type* thread_manager_ptr_type;
  typedef typename ThreadManager::event_handle_type event_handle_type;
  typedef uint64_t handle_type;

  
  TCPClient(std::string const& host, std::string const& port,
    thread_manager_ptr_type thread_manager) :
    thread_manager_(thread_manager), 
    io_service_(thread_manager->io_service()),
    socket_(thread_manager->io_service() ),
    write_mutex_(__LINE__, __FILE__)
  {
    LOG_STACK_TRACE_POINT;
    
    handle_ = next_handle();
    
    writing_ = false;
    Connect(host, port);
  }

  TCPClient(std::string const& host, uint16_t const& port,
    thread_manager_ptr_type thread_manager) :
    thread_manager_(thread_manager),
    io_service_(thread_manager->io_service()),
    socket_(thread_manager->io_service() )    
  
  {
    LOG_STACK_TRACE_POINT;
    
    handle_ = next_handle();

    event_start_service_ = thread_manager_->OnBeforeStart([this]() { this->writing_ = false; });    
    event_stop_service_ = thread_manager_->OnBeforeStop([this]() { this->writing_ = false; });
    
    writing_ = false;    
    Connect(host, port);
    fetch::logger.Debug("Connection established!");
  }


  ~TCPClient()
  {
    LOG_STACK_TRACE_POINT;
    
    thread_manager_->Off( event_start_service_ );
    thread_manager_->Off( event_stop_service_ );        
    socket_.close();

  }

  void Send(message_type const& msg) 
  {
    LOG_STACK_TRACE_POINT;
    
    fetch::logger.Debug("Client: Sending message to server");    
    auto cb = [=]() 
      {
        LOG_LAMBDA_STACK_TRACE_POINT;            
        write_mutex_.lock();
        write_queue_.push_back(msg);
        if (writing_) 
        {
          write_mutex_.unlock();
        } else 
        {
          fetch::logger.Debug("Client: Start writing message");    
          writing_ = true;
          write_mutex_.unlock();
          Write();
        }
      };

    io_service_.post(cb);
  }

  virtual void PushMessage(message_type const& value) = 0;
  virtual void ConnectionFailed() = 0;

  handle_type const &handle() const {    
    return handle_;
  }

  std::string  Address() const 
  {
    return socket_.remote_endpoint().address().to_string();    
  }
  
private:
  void Connect(std::string const& host, std::string const& port) 
  {
    LOG_STACK_TRACE_POINT;    
    asio::ip::tcp::resolver resolver(io_service_);
    Connect(resolver.resolve({host, port}));
  }

  void Connect(std::string const& host, uint16_t const& port) 
  {
    LOG_STACK_TRACE_POINT;    
    std::stringstream p;
    p << port;

    asio::ip::tcp::resolver resolver(io_service_);
    Connect(resolver.resolve({host, p.str()}));
  }

  void Connect(asio::ip::tcp::tcp::resolver::iterator endpoint_iterator) 
  {
    LOG_STACK_TRACE_POINT;    
    auto cb = [=](std::error_code ec,      
      asio::ip::tcp::tcp::resolver::iterator) 
      {
        LOG_LAMBDA_STACK_TRACE_POINT;    
        if (!ec) 
        {
          ReadHeader();
        }
      };
    asio::async_connect(socket_, endpoint_iterator, cb);
  }

  void ReadHeader() 
  {
    LOG_STACK_TRACE_POINT; 
    auto cb = [=](std::error_code ec, std::size_t)      
      {
        LOG_STACK_TRACE_POINT;    // Deliberately breaking the chain
        if (!ec) 
        {
          ReadBody();
        } else 
        {
          fetch::logger.Error("Reading header failed, closing connection: ", ec);
          ConnectionFailed();           
          socket_.close();
        }
      };
    

    asio::async_read(socket_, asio::buffer(header_.bytes, 2*sizeof(uint64_t)),
                     cb);
  }

  void ReadBody() 
  {
    LOG_STACK_TRACE_POINT;    
    byte_array::ByteArray message;
    if( header_.magic != 0xFE7C80A1FE7C80A1) {
      fetch::logger.Debug("Magic incorrect - closing connection.");
      ConnectionFailed();           
      socket_.close();
      return;
    }
    
    message.Resize(header_.length);

    auto cb = [=](std::error_code ec, std::size_t) 
      {
        LOG_LAMBDA_STACK_TRACE_POINT;
        if (!ec) 
        {
          PushMessage(message);
          ReadHeader();
        } else 
        {
          fetch::logger.Error("Reading body failed, closing connection: ", ec);
          ConnectionFailed();        
          socket_.close();
        }
      };

    asio::async_read(socket_, asio::buffer(message.pointer(), message.size()),
      cb);
  }

  void Write() {
    LOG_STACK_TRACE_POINT;    
    serializers::ByteArrayBuffer buffer;

    write_mutex_.lock();
    if( write_queue_.empty() ) 
    {
      fetch::logger.Debug("Queue is empty stopping");    
      write_mutex_.unlock();
      return;
    }

    buffer << 0xFE7C80A1FE7C80A1;
    buffer << write_queue_.front();
    write_mutex_.unlock();
    
    auto cb = [=](std::error_code ec, std::size_t) 
      {
        LOG_STACK_TRACE_POINT; // Deliberately breaking the chain

        if (!ec) 
        {
          fetch::logger.Debug("Wrote message.");     
          write_mutex_.lock();
          write_queue_.pop_front();
          bool should_write = writing_ = !write_queue_.empty();
          write_mutex_.unlock();
        
          if (should_write) 
          {
            fetch::logger.Debug("Proceeding to next.");
          
            Write();
          } 
        } else 
        {
          fetch::logger.Error("Client: Write failed, closing connection:", ec);
          ConnectionFailed();                
          socket_.close();
        }
      };


    asio::async_write(socket_, asio::buffer(buffer.data().pointer(), buffer.data().size()),
      cb);

  }


private:
  event_handle_type event_start_service_;
  event_handle_type event_stop_service_;      
  thread_manager_ptr_type thread_manager_;

  handle_type handle_;
  static handle_type global_handle_counter_;
  static fetch::mutex::Mutex global_handle_mutex_;
  static handle_type next_handle() 
  {
    std::lock_guard<fetch::mutex::Mutex> lck(global_handle_mutex_);
    handle_type ret = global_handle_counter_;
    ++global_handle_counter_;
    return ret;
  }

  union {
    char bytes[2*sizeof(uint64_t)];
    struct {
      uint64_t magic;
      uint64_t length;
    };

  } header_;

  asio::io_service &io_service_;
  asio::ip::tcp::tcp::socket socket_;


  bool writing_ = false;
  message_queue_type write_queue_;
  fetch::mutex::Mutex write_mutex_;

};

TCPClient::handle_type
TCPClient::global_handle_counter_ = 0;
fetch::mutex::Mutex TCPClient::global_handle_mutex_(__LINE__, __FILE__);

};
};

#endif
