#ifndef NETWORK_TCP_SERVER_HPP
#define NETWORK_TCP_SERVER_HPP

#include "network/thread_manager.hpp"
#include "network/tcp/client_connection.hpp"
#include "mutex.hpp"

#include <asio.hpp>
#include <deque>
#include <mutex>
#include <thread>

namespace fetch {
namespace network {
class TCPServer : public AbstractNetworkServer {
 public:
  typedef uint64_t handle_type;

  typedef ThreadManager thread_manager_type;  
  typedef thread_manager_type* thread_manager_ptr_type;
  typedef typename ThreadManager::event_handle_type event_handle_type;
   
  struct Request {
    handle_type handle;
    message_type meesage;
  };

  TCPServer(uint16_t const& port, thread_manager_ptr_type const &thread_manager) :
    thread_manager_(thread_manager),
    request_mutex_(__LINE__, __FILE__),
    acceptor_(thread_manager->io_service(), asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
    socket_(thread_manager->io_service())
  {

    event_service_start_ = thread_manager->OnBeforeStart([this]() { this->Accept(); } ); 
    manager_ = new ClientManager(*this);    
  }

  ~TCPServer() {
    thread_manager_->Off(event_service_start_);
    socket_.close();
  }

  void PushRequest(handle_type client, message_type const& msg) override {
    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    requests_.push_back({client, msg});
  }

  void Broadcast(message_type const& msg) {
    manager_->Broadcast(msg);
  }
  
  bool Send(handle_type const &client, message_type const& msg) {
    return manager_->Send(client, msg);
  }

  bool has_requests() {
    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    bool ret = (requests_.size() != 0);
    return ret;
  }

  /**
     @brief returns the top request.
  **/
  Request Top() {
    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    Request top = requests_.front();
    return top;
  }

  /**
     @brief returns the pops the top request.
  **/  
  void Pop() {
    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    requests_.pop_front();
  }

  std::string GetAddress(handle_type const &client) const
  {
    return manager_->GetAddress(client);    
  }
  

  
 private:
  thread_manager_ptr_type thread_manager_;
  event_handle_type event_service_start_;  
  std::deque<Request> requests_;
  fetch::mutex::Mutex request_mutex_;

  void Accept() {
    auto cb = [this](std::error_code ec) {

      if (!ec) {
        std::make_shared<ClientConnection>(std::move(socket_), *manager_)
            ->Start();
      }

      Accept();
    };

    acceptor_.async_accept(socket_, cb);
  }

  static handle_type global_handle_counter_;
  static fetch::mutex::Mutex global_handle_mutex_;


  asio::ip::tcp::tcp::acceptor acceptor_;
  asio::ip::tcp::tcp::socket socket_;
  ClientManager* manager_;
};
};
};

#endif
