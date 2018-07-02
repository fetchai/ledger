#ifndef NETWORK_TCP_SERVER_HPP
#define NETWORK_TCP_SERVER_HPP

#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/tcp/client_connection.hpp"
#include "network/details/thread_manager.hpp"

#include "network/fetch_asio.hpp"

#include <deque>
#include <mutex>
#include <thread>

namespace fetch {
namespace network {

class TCPServer : public AbstractNetworkServer {
 public:
  typedef uint64_t handle_type;

  typedef ThreadManager thread_manager_type;
  typedef typename ThreadManager::event_handle_type event_handle_type;

  struct Request {
    handle_type handle;
    message_type meesage;
  };


  TCPServer(uint16_t const port, const thread_manager_type thread_manager)
    : request_mutex_(__LINE__, __FILE__)
  {
    std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ MAKERING1" << " " << port << std::endl;
    thread_manager_ = thread_manager;
      std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ MAKERING2" << std::endl;
      acceptor_ = std::make_shared<asio::ip::tcp::tcp::acceptor>(thread_manager_.io_service(), asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
    std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ MAKERING3" << std::endl;
    socket_ = std::make_shared<asio::ip::tcp::tcp::socket>(thread_manager_.io_service());
      std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ MAKERING4" << std::endl;

      std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ MAKERING5" << std::endl;
    LOG_STACK_TRACE_POINT;
    //event_service_start_ =
    //    thread_manager_.OnBeforeStart([this]() { this->Accept(); });
        std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ POSTING" << std::endl;

    thread_manager_.Post([this]{
         std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ IN POSTED TASK" << std::endl;
     this->Accept();
    });

    usleep(10000);

    // TODO: If manager running -> Accept();
    manager_ = new ClientManager(*this);
  }

  ~TCPServer() {
    LOG_STACK_TRACE_POINT;
    //thread_manager_->Off(event_service_start_);
    if (manager_ != nullptr) delete manager_;
    socket_ -> close();
  }

  void PushRequest(handle_type client, message_type const& msg) override {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("Got request from ", client);

    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    requests_.push_back({client, msg});
  }

  void Broadcast(message_type const& msg) {
    LOG_STACK_TRACE_POINT;
    manager_->Broadcast(msg);
  }

  bool Send(handle_type const& client, message_type const& msg) {
    LOG_STACK_TRACE_POINT;
    return manager_->Send(client, msg);
  }

  bool has_requests() {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    bool ret = (requests_.size() != 0);
    return ret;
  }

  /**
     @brief returns the top request.
  **/
  Request Top() {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    Request top = requests_.front();
    return top;
  }

  /**
     @brief returns the pops the top request.
  **/
  void Pop() {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    requests_.pop_front();
  }

  std::string GetAddress(handle_type const& client) const {
    LOG_STACK_TRACE_POINT;
    return manager_->GetAddress(client);
  }

 private:
  //thread_manager_ptr_type thread_manager_;
  //event_handle_type event_service_start_;

  void Accept() {
    LOG_STACK_TRACE_POINT;
          std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ ACCEPTING 1" << std::endl;
    auto cb = [=](std::error_code ec) {
          std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ ACCEPTING 2" << std::endl;
      LOG_LAMBDA_STACK_TRACE_POINT;
      if (!ec) {
        std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ ACCEPTING 3" << std::endl;
        std::make_shared<ClientConnection>(std::move(*socket_), *manager_)
            ->Start();
      } else
        {
          std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ ACCEPTING ERK!" << ec << std::endl;
        }

      std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ calling ACCEPT" << std::endl;
      Accept();
    };

      std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ calling async ACCEPT" << std::endl;
      acceptor_ -> async_accept(*socket_, cb) ;
      std::cout << "$$$$$$$$$$$$$$$$$$$$$$$ sdon calling async ACCEPT" << std::endl;
  }

  thread_manager_type thread_manager_;
  fetch::mutex::Mutex request_mutex_;
  std::shared_ptr<asio::ip::tcp::tcp::acceptor> acceptor_;
  std::shared_ptr<asio::ip::tcp::tcp::socket> socket_;
  std::deque<Request> requests_;
  ClientManager* manager_;
};
}
}

#endif
