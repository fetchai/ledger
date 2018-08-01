#pragma once

#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/management/connection_register.hpp"
#include "network/management/network_manager.hpp"
#include "network/tcp/client_connection.hpp"

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

class TCPServer : public AbstractNetworkServer
{
public:
  typedef typename AbstractConnection::connection_handle_type connection_handle_type;
  typedef NetworkManager                                      network_manager_type;
  typedef asio::ip::tcp::tcp::acceptor                        acceptor_type;
  typedef std::mutex                                          mutex_type;

  struct Request
  {
    connection_handle_type handle;
    message_type           message;
  };

  TCPServer(uint16_t const &port, network_manager_type network_manager)
      : network_manager_{network_manager}, port_{port}, request_mutex_{}
  {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Info("Creating TCP server");
    manager_ = std::make_shared<ClientManager>(*this);

    std::shared_ptr<int> destruct_guard = destruct_guard_;

    auto closure = [this, destruct_guard] {
      std::lock_guard<std::mutex> lock(startMutex_);

      if (!stopping_)
      {
        std::shared_ptr<acceptor_type> acceptor;

        try
        {// KLL this also appears to generate a data race.
          // This might throw if the port is not free
          acceptor = network_manager_.CreateIO<acceptor_type>(
              asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port_));

          acceptor_ = acceptor;

          fetch::logger.Info("Starting TCP server acceptor loop");
          acceptor_ = acceptor;

          if (acceptor)
          {
            running_ = true;
            Accept(acceptor);
            fetch::logger.Info("Accepting TCP server connections");
          }
        }
        catch (std::exception &e)
        {
          fetch::logger.Info("Failed to open socket: ", port_, " with error: ", e.what());
        }
      }
    };

    network_manager.Post(closure);
  }

  virtual ~TCPServer() override
  {
    LOG_STACK_TRACE_POINT;

    {
      std::lock_guard<std::mutex> lock(startMutex_);
      stopping_ = true;
    }

    std::weak_ptr<acceptor_type> acceptorWeak = acceptor_;

    network_manager_.Post([acceptorWeak] {
      auto acceptorStrong = acceptorWeak.lock();
      if (acceptorStrong)
      {
        std::error_code dummy;
        acceptorStrong->close(dummy);
        fetch::logger.Info("closed TCP server server acceptor: ");
      }
      else
      {
        fetch::logger.Info("failed to close acceptor: ");
      }
    });

    while (destruct_guard_.use_count() > 1)
    {
      fetch::logger.Info("Waiting for TCP server ", this, " start closure to clear");
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    while (!acceptor_.expired() && running_)
    {
      fetch::logger.Info("Waiting for TCP server ", this, " to destruct");
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    fetch::logger.Info("Destructing TCP server ", this);
  }

  void PushRequest(connection_handle_type client, message_type const &msg) override
  {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("Got request from ", client);

    std::lock_guard<mutex_type> lock(request_mutex_);
    requests_.push_back({client, msg});
  }

  void Broadcast(message_type const &msg)
  {
    LOG_STACK_TRACE_POINT;
    manager_->Broadcast(msg);
  }

  bool Send(connection_handle_type const &client, message_type const &msg)
  {
    LOG_STACK_TRACE_POINT;
    return manager_->Send(client, msg);
  }

  bool has_requests()
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<mutex_type> lock(request_mutex_);
    bool                        ret = (requests_.size() != 0);
    return ret;
  }

  /**
     @brief returns the top request.
  **/
  Request Top()
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<mutex_type> lock(request_mutex_);
    Request                     top = requests_.front();
    return top;
  }

  void Pop()
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<mutex_type> lock(request_mutex_);
    requests_.pop_front();
  }

  std::string GetAddress(connection_handle_type const &client)
  {
    LOG_STACK_TRACE_POINT;
    return manager_->GetAddress(client);
  }

  template <typename X>
  void SetConnectionRegister(X &reg)
  {
    connection_register_ = reg.pointer();
  }

private:
  network_manager_type network_manager_;
  uint16_t             port_;
  std::deque<Request>  requests_;
  mutex_type           request_mutex_;

  void Accept(std::shared_ptr<asio::ip::tcp::tcp::acceptor> acceptor)
  {
    LOG_STACK_TRACE_POINT;

    auto strongSocket                = network_manager_.CreateIO<asio::ip::tcp::tcp::socket>();
    std::weak_ptr<ClientManager> man = manager_;

    auto cb = [this, man, acceptor, strongSocket](std::error_code ec) {
      auto lock_ptr = man.lock();
      if (!lock_ptr) return;

      if (!ec)
      {
        auto conn = std::make_shared<ClientConnection>(strongSocket, manager_);
        auto ptr  = connection_register_.lock();

        if (ptr)
        {
          ptr->Enter(conn->connection_pointer());
          conn->SetConnectionManager(ptr);
        }

        conn->Start();
        Accept(acceptor);
      }
      else
      {
        fetch::logger.Info("Acceptor in TCP server received EC");
      }
    };

    acceptor->async_accept(*strongSocket, cb);
  }

  std::shared_ptr<int>                      destruct_guard_ = std::make_shared<int>(0);
  std::weak_ptr<AbstractConnectionRegister> connection_register_;
  std::shared_ptr<ClientManager>            manager_;
  std::weak_ptr<acceptor_type>              acceptor_;
  std::mutex                                startMutex_;
  bool                                      stopping_ = false;
  bool                                      running_  = false;
};
}  // namespace network
}  // namespace fetch
