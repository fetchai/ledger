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
  typedef thread_manager_type* thread_manager_ptr_type;
  typedef typename ThreadManager::event_handle_type event_handle_type;
  typedef uint64_t handle_type;

  TCPClientImplementation(thread_manager_ptr_type thread_manager) noexcept
      : thread_manager_(*thread_manager),
        io_service_(thread_manager->io_service()),
        resolver_(io_service_),
        socket_(thread_manager->io_service())

  {
    LOG_STACK_TRACE_POINT;

    handle_ = next_handle();

    event_start_service_ =
        thread_manager_.OnBeforeStart([this]() { this->writing_ = false; });
    event_stop_service_ =
        thread_manager_.OnBeforeStop([this]() { this->writing_ = false; });

    writing_ = false;
  }

  ~TCPClientImplementation() noexcept {
    LOG_STACK_TRACE_POINT;

    std::cerr << "Setting off. " << std::endl;
    thread_manager_.Off(event_start_service_);
    thread_manager_.Off(event_stop_service_);
    Close();
  }

  void Send(message_type const& msg) noexcept {
    LOG_STACK_TRACE_POINT;

    fetch::logger.Debug("Client: Sending message to server");
    auto self = shared_from_this();
    auto cb = [this,self, msg]() {

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
    return socket_.remote_endpoint().address().to_string();
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
    endpoint_iterator_ = resolver_.resolve({std::string(host), std::string(port)});
    Connect();
  }

  void Connect(byte_array::ConstByteArray const& host,
               uint16_t const& port) noexcept {
    LOG_STACK_TRACE_POINT;
    std::stringstream p;
    p << port;

    endpoint_iterator_ = resolver_.resolve({std::string(host), p.str()});
    Connect();
  }

  void Close(bool failed = false) {
    if (is_alive_) {
      is_alive_ = false;
      std::lock_guard<fetch::mutex::Mutex> lock(leave_mutex_);

      if (on_leave_) {
        on_leave_();
      }

      if (failed) {
        std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
      }
    }

    while(1)
    {
      std::unique_lock<fetch::mutex::Mutex> lock(connection_state_mutex_);
      if(classState == State::closed)
      {
        break;
      }

      if(classState == State::connecting)
      {
        lock.unlock();
        fetch::logger.Info("Client close waiting for socket to finish connecting");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }

      // ignoring ec will disable exceptions
      std::error_code ignore_ec;
      socket_.shutdown(asio::ip::tcp::tcp::socket::shutdown_both, ignore_ec);
      socket_.close(ignore_ec);
      classState = State::closed;
    }
  }

 private:
  mutable fetch::mutex::Mutex callback_mutex_;

  std::function< void(message_type const&) >  on_push_message_;
  std::function< void() > on_connection_failed_;

  void Connect( ) noexcept {
    LOG_STACK_TRACE_POINT;
    auto self = shared_from_this();
    auto cb = [this,self](std::error_code ec, asio::ip::tcp::tcp::resolver::iterator) {

      {
        std::unique_lock<fetch::mutex::Mutex> lock(connection_state_mutex_);
        classState = State::connected;
      }
      is_alive_ = true;

      LOG_STACK_TRACE_POINT;
      if (!ec) {
        fetch::logger.Debug("Connection established!");
        ReadHeader();
      } else {
        if (is_alive_) {
          std::lock_guard< fetch::mutex::Mutex > lock(callback_mutex_);
          if(on_connection_failed_) on_connection_failed_();

        }

        is_alive_ = false;
      }

    };

    std::unique_lock<fetch::mutex::Mutex> lock(connection_state_mutex_);
    if(classState == State::unconnected)
    {
      classState = State::connecting;
      asio::async_connect(socket_, endpoint_iterator_, cb);
    }
  }

  void ReadHeader() noexcept {
    LOG_STACK_TRACE_POINT;
    auto self = shared_from_this();
    auto cb = [this,self](std::error_code ec, std::size_t) {
      if (!ec) {
        ReadBody();
      } else {
        if(is_alive_) {
          Close(true);
          fetch::logger.Error("Reading header failed, closing connection: ", ec);
        }

      }
    };

    asio::async_read(socket_, asio::buffer(header_.bytes, 2 * sizeof(uint64_t)),
                     cb);
  }

  void ReadBody() noexcept {
    byte_array::ByteArray message;
    if (header_.content.magic != networkMagic) {
      fetch::logger.Debug("Magic incorrect during network read");

      if(is_alive_) {
        fetch::logger.Debug("Magic incorrect - closing connection.");
        Close(true);
      }

      return;
    }

    message.Resize(header_.content.length);

    auto self = shared_from_this();
    auto cb = [this,self, message](std::error_code ec, std::size_t len) {

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

    asio::async_read(socket_, asio::buffer(message.pointer(), message.size()),
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

    auto self = shared_from_this();
    auto cb = [this,self, buffer, header](std::error_code ec, std::size_t) {

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

      asio::async_write( socket_, buffers, cb);
    }
  }

 private:
  event_handle_type event_start_service_;
  event_handle_type event_stop_service_;
  thread_manager_type thread_manager_;

  asio::io_service&                      io_service_;
  asio::ip::tcp::resolver                resolver_;
  asio::ip::tcp::tcp::socket             socket_;
  asio::ip::tcp::tcp::resolver::iterator endpoint_iterator_;

  std::atomic<bool> is_alive_;
  mutable fetch::mutex::Mutex leave_mutex_;
  std::function<void()> on_leave_;
  const uint64_t networkMagic = 0xFE7C80A1FE7C80A1;

  // Class state can only move -> in enum
  mutable fetch::mutex::Mutex connection_state_mutex_;
  enum class State { unconnected, connecting, connected, closed };
  State classState = State::unconnected;

  handle_type handle_;
  static handle_type global_handle_counter_;
  static fetch::mutex::Mutex global_handle_mutex_;
  static handle_type next_handle() {
    std::lock_guard<fetch::mutex::Mutex> lck(global_handle_mutex_);
    handle_type ret = global_handle_counter_;
    ++global_handle_counter_;
    return ret;
  }

  union {
    char bytes[2 * sizeof(uint64_t)];
    struct {
      uint64_t magic;
      uint64_t length;
    } content;

  } header_;

  bool writing_ = false;
  message_queue_type write_queue_;
  fetch::mutex::Mutex write_mutex_;
};

TCPClientImplementation::handle_type TCPClientImplementation::global_handle_counter_ = 0;
fetch::mutex::Mutex TCPClientImplementation::global_handle_mutex_(__LINE__, __FILE__);


}
}

#endif
