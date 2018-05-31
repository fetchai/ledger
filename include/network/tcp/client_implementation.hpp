#ifndef NETWORK_TCP_CLIENT_IMPLEMENTATION_HPP
#define NETWORK_TCP_CLIENT_IMPLEMENTATION_HPP

#include "byte_array/const_byte_array.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "logger.hpp"
#include "network/message.hpp"
#include "network/thread_manager.hpp"
#include "serializers/byte_array_buffer.hpp"
#include "serializers/referenced_byte_array.hpp"

#include "mutex.hpp"

#include "fetch_asio.hpp"

#include <atomic>
#include <memory>
#include <mutex>

namespace fetch {
namespace network {

class TCPClientImplementation : public std::enable_shared_from_this< TCPClientImplementation > {
 public:
  typedef ThreadManager thread_manager_type;

  typedef typename ThreadManager::event_handle_type event_handle_type;
  typedef uint64_t handle_type;
  typedef std::weak_ptr< TCPClientImplementation > weak_ptr_type;
  typedef std::shared_ptr< TCPClientImplementation > shared_ptr_type;
  
  TCPClientImplementation(thread_manager_type & thread_manager)
      : thread_manager_(thread_manager),
        io_service_(thread_manager.io_service())
  {
    LOG_STACK_TRACE_POINT;
    
    handle_ = next_handle();

    {
      auto tmlock = thread_manager_.lock();
      if(!tmlock) return;
      event_start_service_ =
        thread_manager_.OnBeforeStart([this]() { this->writing_ = false; });
      event_stop_service_ =
        thread_manager_.OnBeforeStop([this]() {
            this->writing_ = false;
            //            Close();
          });
    }
    writing_ = false;

    socket_ = std::make_shared<asio::ip::tcp::tcp::socket> (thread_manager.io_service()) ;
  }

  ~TCPClientImplementation() noexcept {
    LOG_STACK_TRACE_POINT;
    auto tmlock = thread_manager_.lock();
    if(!tmlock) return;

    thread_manager_.Off(event_start_service_);
    thread_manager_.Off(event_stop_service_);

    
    std::lock_guard<fetch::mutex::Mutex> lock(close_mutex_);


      auto soc = socket_;
      thread_manager_.io_service().post([soc]() {
          if(soc->is_open()) {
            try {
              std::error_code ec;
              soc->shutdown(asio::ip::tcp::tcp::socket::shutdown_both, ec);  
              soc->close();

            } catch(...) { }
          }
        });

  }

  void Send(message_type const& msg) noexcept {
    auto tmlock = thread_manager_.lock();
    if(!tmlock) return;
    
    LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("Client: Sending message to server");
    weak_ptr_type self = shared_from_this();

    auto cb = [this,self, msg]() {
      shared_ptr_type shared_self = self.lock();
      if(!shared_self) {
        return;
      }


      
      LOG_STACK_TRACE_POINT;
      write_mutex_.lock();
      write_queue_.push_back(msg);
      if (writing_) {
        write_mutex_.unlock();
      } else {
        fetch::logger.Debug("Client: Start writing message");
        bool write;
        if (is_alive_)
          write = writing_ = true;
        else
          write = writing_ = false;

        write_mutex_.unlock();
        if (write) Write();
      }
    };

    if (is_alive_) io_service_.post(cb);
  }

//  void PushMessage(message_type const& value) = 0;
//  virtual void ConnectionFailed() = 0;

  handle_type const& handle() const noexcept { return handle_; }

  std::string Address() const noexcept {
    return socket_->remote_endpoint().address().to_string();
  }

  void OnLeave(std::function<void()> fnc) {
    std::lock_guard<fetch::mutex::Mutex> lock(leave_mutex_);

    on_leave_ = fnc;
  }

  void ClearLeave() {
    std::lock_guard<fetch::mutex::Mutex> lock(leave_mutex_);
    on_leave_ = nullptr;
  }


  bool is_alive() const noexcept { return is_alive_; }


  void OnConnectionFailed(std::function< void() > const &fnc)
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
    on_connection_failed_ = fnc;
  }

  void ClearConnectionFailed()
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
    on_connection_failed_  = nullptr;
  }

  void OnPushMessage(std::function< void(message_type const&) > const &fnc)
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
    on_push_message_ = fnc;
  }

  void ClearPushMessage()
  {
    std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
    on_push_message_ = nullptr;
  }

  void Connect(byte_array::ConstByteArray const& host,
               byte_array::ConstByteArray const& port) noexcept {
    LOG_STACK_TRACE_POINT;
    Connect(std::string(host), std::string(port));      
  }

  void Connect(byte_array::ConstByteArray const& host,
               uint16_t const& port) noexcept {
    LOG_STACK_TRACE_POINT;
    std::stringstream p;
    p << port;

    Connect(host, p.str());
  }
                   

  void Close(bool failed = false) {
    //std::cerr << "Closing client" << std::endl;
    auto tmlock = thread_manager_.lock();
    if(!tmlock) return;
    
    std::lock_guard<fetch::mutex::Mutex> lock(close_mutex_);


    if (is_alive_) {
      is_alive_ = false;
      weak_ptr_type self;
      try {
       self = shared_from_this();
      } catch (std::bad_weak_ptr const& e) {
        return;
      }

      auto soc = socket_;
      auto cb = [this,self, failed, soc]() {
        shared_ptr_type shared_self = self.lock();
        if(!shared_self) {
          return;
        }  

        std::lock_guard<fetch::mutex::Mutex> lock(close_mutex_);
        
        if (on_leave_) {
          std::lock_guard<fetch::mutex::Mutex> lock(leave_mutex_);
          on_leave_();
        }
        
        if (failed) {
          std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
          if(on_connection_failed_) {
            on_connection_failed_();
          }
        }

        if(soc->is_open()) {
          std::error_code ec;
          soc->shutdown(asio::ip::tcp::tcp::socket::shutdown_both, ec);
          soc->close();
        }
      };
      
      io_service_.post(cb);
    }

  }

 private:
  mutable fetch::mutex::Mutex callback_mutex_;

  std::function< void(message_type const&) >  on_push_message_;
  std::function< void() > on_connection_failed_;

  void Connect(std::string const&host, std::string const &port) noexcept {
    auto tmlock = thread_manager_.lock();
    if(!tmlock) return;
    
    LOG_STACK_TRACE_POINT;

    weak_ptr_type self;
    try {
      self = shared_from_this();
    } catch (std::bad_weak_ptr const& e) {
      return;
    }

    auto soc = socket_;
    asio::ip::tcp::resolver resolver(thread_manager_.io_service());
    auto cb = [this, self, tmlock, &resolver, host, port, soc](std::error_code ec, asio::ip::tcp::tcp::resolver::iterator) {
      shared_ptr_type shared_self = self.lock();
      if(!shared_self) {
        return;
      }
     
      LOG_STACK_TRACE_POINT;
      if (!ec) {
        is_alive_ = true;
        fetch::logger.Debug("Connection established!");
        ReadHeader();
      } else {
        is_alive_ = false;
        // No callbacks since we are in the object construction process
        return; 
      }

    };

    asio::ip::tcp::tcp::resolver::iterator endpoint_iterator;

    try {
      endpoint_iterator = resolver.resolve({host, port});
    } catch(...) {
      //      std::__1::system_error: resolve: Host not found (authoritative)      
      return;
    }
        
    try {
      asio::async_connect(*socket_, endpoint_iterator, cb);
    } catch(...) {
      is_alive_ = false;
      return;
    }

  }

  void ReadHeader() noexcept {
    LOG_STACK_TRACE_POINT;    
    weak_ptr_type self = shared_from_this();
    
    byte_array::ByteArray header;
    header.Resize(2 * sizeof(uint64_t) );

    auto soc = socket_;    
    auto cb = [this, self, header, soc](std::error_code ec, std::size_t) {
      shared_ptr_type shared_self = self.lock();
      if(!shared_self) {
        return;
      }

      if (!ec) {
        ReadBody(header);
      } else {
        if(is_alive_) {
          Close(true);
          fetch::logger.Error("Reading header failed, closing connection: ", ec);
        }

      }
    };

    asio::async_read(*socket_, asio::buffer(header.pointer(), header.size()),
                     cb);
  }

  void ReadBody(byte_array::ByteArray const &h) noexcept {
    byte_array::ByteArray message;

    HeaderUnion header;
    for(std::size_t i = 0; i < h.size(); ++i) {
      header.bytes[i] = h[i];
    }
    
    if (header.content.magic != networkMagic) {
      fetch::logger.Debug("Magic incorrect during network read");

      if(is_alive_) {
        fetch::logger.Debug("Magic incorrect - closing connection.");
        Close(true);
      }

      return;
    }

    message.Resize(header.content.length);

    weak_ptr_type self = shared_from_this();
    auto soc = socket_;
    auto cb = [this,self, message, soc](std::error_code ec, std::size_t len) {
      shared_ptr_type shared_self = self.lock();
      if(!shared_self) {
        return;
      }

      if (!ec) {
        {
          std::lock_guard< fetch::mutex::Mutex > lock( callback_mutex_ );
          if(on_push_message_) on_push_message_(message);
        }
        ReadHeader();
      } else {
        if(is_alive_) {
          fetch::logger.Error("Reading body failed, closing connection: ", ec);
          Close(true);
        }

      }
    };

    
    asio::async_read(*socket_, asio::buffer(message.pointer(), message.size()),
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

  void Write() noexcept {
    LOG_STACK_TRACE_POINT;

    write_mutex_.lock();
    if (write_queue_.empty()) {
      fetch::logger.Debug("Network write queue is empty, stopping");
      write_mutex_.unlock();
      return;
    }

    auto buffer          = write_queue_.front();
    write_queue_.pop_front();
    byte_array::ByteArray header;
    SetHeader(header, buffer.size());
    write_mutex_.unlock();

    weak_ptr_type self = shared_from_this();
    auto soc = socket_;
    auto cb = [this,self, buffer, header, soc](std::error_code ec, std::size_t) {
      shared_ptr_type shared_self = self.lock();
      if(!shared_self) {
        return;
      }
      
      if (!ec) {
        fetch::logger.Debug("Wrote message.");
        write_mutex_.lock();

        bool should_write = writing_ = !write_queue_.empty();
        write_mutex_.unlock();

        if (should_write && is_alive_) {
          fetch::logger.Debug("Proceeding to next.");

          Write();
        }
      } else {
        if(is_alive_) {
          fetch::logger.Error("Client: Write failed, closing connection:", ec);
          Close(true);
        }
      }
    };

    if (is_alive_) {
      std::vector<asio::const_buffer> buffers{
        asio::buffer(header.pointer(), header.size()),
        asio::buffer(buffer.pointer(), buffer.size())};


      asio::async_write( *socket_, buffers, cb);
    }
  }

 private:
  std::atomic<bool> is_alive_;
  mutable fetch::mutex::Mutex leave_mutex_, close_mutex_;
  std::function<void()> on_leave_;
  const uint64_t networkMagic = 0xFE7C80A1FE7C80A1;

  event_handle_type event_start_service_;
  event_handle_type event_stop_service_;
  thread_manager_type thread_manager_;

  handle_type handle_;
  static handle_type global_handle_counter_;
  static fetch::mutex::Mutex global_handle_mutex_;
  static handle_type next_handle() {
    std::lock_guard<fetch::mutex::Mutex> lck(global_handle_mutex_);
    handle_type ret = global_handle_counter_;
    ++global_handle_counter_;
    return ret;
  }

  union HeaderUnion {
    uint8_t bytes[2 * sizeof(uint64_t)];
    struct {
      uint64_t magic;
      uint64_t length;
    } content;

  };

  asio::io_service& io_service_;
  std::shared_ptr<asio::ip::tcp::tcp::socket> socket_;
  
  bool writing_ = false;
  message_queue_type write_queue_;
  fetch::mutex::Mutex write_mutex_;
};

TCPClientImplementation::handle_type TCPClientImplementation::global_handle_counter_ = 0;
fetch::mutex::Mutex TCPClientImplementation::global_handle_mutex_(__LINE__, __FILE__);


}
}

#endif
