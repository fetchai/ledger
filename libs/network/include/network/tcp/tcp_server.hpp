#ifndef NETWORK_TCP_SERVER_HPP
#define NETWORK_TCP_SERVER_HPP

#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/tcp/client_connection.hpp"
#include "network/management/network_manager.hpp"
#include "network/tcp/client_connection.hpp"
#include "network/management/connection_register.hpp"

#include "network/fetch_asio.hpp"

#include <deque>
#include <mutex>
#include <thread>

namespace fetch {
namespace network {

/*
 * Handle TCP connections. Spawn new ClientConnections on connect and adds
 * them to the client manager. This can then be used for communication with
 * the rest of Fetch
 *
 */

class TCPServer : public AbstractNetworkServer {
 public:
  typedef typename AbstractConnection::connection_handle_type connection_handle_type;
  typedef NetworkManager                                      network_manager_type;
  typedef asio::ip::tcp::tcp::acceptor                        acceptor_type;

  struct Request {
    connection_handle_type handle;
    message_type           message;
  };

  TCPServer(uint16_t const& port, network_manager_type network_manager) :
        network_manager_{network_manager},
        port_{port},
        request_mutex_(__LINE__, __FILE__)
  {
    LOG_STACK_TRACE_POINT;
    manager_ = std::make_shared< ClientManager >(*this);

    std::shared_ptr<acceptor_type> acceptor =
      network_manager_.CreateIO<asio::ip::tcp::tcp::acceptor>
      (asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port_));

    acceptor_ = acceptor;

    auto closure = [this, acceptor]
    {
      std::cout << "starting: " << acceptor.use_count() << std::endl;
      std::lock_guard<std::mutex> lock(startMutex_);
      if(!stopping_)
      {
        running_ = true;
        Accept(acceptor);
      }
    };

    acceptor.reset(); // The acceptor should be kept alive in the closure

    network_manager.Post(closure);
  }

  virtual ~TCPServer() {
    LOG_STACK_TRACE_POINT;

    {
      std::lock_guard<std::mutex> lock(startMutex_);
      stopping_ = true;
    }

    std::weak_ptr<acceptor_type> acceptorWeak = acceptor_;

    network_manager_.Post([acceptorWeak]
    {
      auto acceptorStrong = acceptorWeak.lock();
      if(acceptorStrong)
      {
        std::error_code dummy;
        acceptorStrong->close(dummy);
      }
    });

    while(!acceptor_.expired() && running_)
    {
      fetch::logger.Info("Waiting for acceptor to die in TCP server. ");
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  void PushRequest(connection_handle_type client, message_type const& msg) override {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("Got request from ", client);

    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    requests_.push_back({client, msg});
  }

  void Broadcast(message_type const& msg) {
    LOG_STACK_TRACE_POINT;
    manager_->Broadcast(msg);
  }

  bool Send(connection_handle_type const& client, message_type const& msg) {
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

  void Pop() {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(request_mutex_);
    requests_.pop_front();
  }

  std::string GetAddress(connection_handle_type const& client) {
    LOG_STACK_TRACE_POINT;
    return manager_->GetAddress(client);
  }

  template<typename X >
  void SetConnectionRegister(X &reg)
  {
    connection_register_ = reg.pointer();
  }

 private:
  network_manager_type       network_manager_;
  uint16_t                   port_;
  std::deque<Request>        requests_;
  fetch::mutex::Mutex        request_mutex_;

  void Accept(std::shared_ptr<asio::ip::tcp::tcp::acceptor> acceptor) {
    LOG_STACK_TRACE_POINT;

    auto strongSocket = network_manager_.CreateIO<asio::ip::tcp::tcp::socket>();
    std::weak_ptr< ClientManager >  man = manager_;

    auto cb = [this, man, acceptor, strongSocket](std::error_code ec) {
      auto lock_ptr = man.lock();
      if(!lock_ptr) return;

      if (!ec) {
        auto conn = std::make_shared<ClientConnection>(strongSocket, manager_);
        auto ptr = connection_register_.lock();

        if(ptr) {
          ptr->Enter( conn->network_client_pointer() );
          conn->SetConnectionManager( ptr );
        }

        conn->Start();
      }

      Accept(acceptor);
    };

    acceptor->async_accept(*strongSocket, cb);
  }

  std::weak_ptr<AbstractConnectionRegister> connection_register_;
  std::shared_ptr<ClientManager>            manager_;
  std::weak_ptr<acceptor_type>              acceptor_;
  std::mutex                                startMutex_;
  bool                                      stopping_ = false;
  bool                                      running_  = false;
};
}
}

#endif
