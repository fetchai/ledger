#ifndef NETWORK_NETWORK_SERVER_HPP
#define NETWORK_NETWORK_SERVER_HPP

#include "network/client_connection.hpp"
#include "mutex.hpp"

#include <asio.hpp>
#include <deque>
#include <mutex>
#include <thread>

namespace fetch {
namespace network {
class NetworkServer : public AbstractNetworkServer {
 public:
  typedef uint64_t handle_type;

  struct Request {
    handle_type handle;
    message_type meesage;
  };

  NetworkServer(uint16_t port)
      : acceptor_(io_service_,
                  asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
        socket_(io_service_), request_mutex_(__LINE__, __FILE__) {
    manager_ = new ClientManager(*this);
  }

  ~NetworkServer() {
    Stop();
    socket_.close();
  }

  virtual void Start() {
    if (thread_ == nullptr) {
      Accept();
      thread_ = new std::thread([this]() { io_service_.run(); });
    }
  }

  virtual void Stop() {
    if (thread_ != nullptr) {
      io_service_.stop();
      thread_->join();
      delete thread_;
      thread_ = nullptr;
    }
  }

  void PushRequest(handle_type client, message_type const& msg) override {
    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    requests_.push_back({client, msg});
  }

  void Respond(handle_type client, message_type const& msg) {
    manager_->Send(client, msg);
  }

  bool has_requests() {
    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    bool ret = (requests_.size() != 0);
    return ret;
  }

  Request Top() {
    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    Request top = requests_.front();
    return top;
  }

  void Pop() {
    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    requests_.pop_front();
  }

 private:
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

  std::thread* thread_ = nullptr;
  asio::io_service io_service_;
  asio::ip::tcp::tcp::acceptor acceptor_;
  asio::ip::tcp::tcp::socket socket_;
  ClientManager* manager_;
};
};
};

#endif
