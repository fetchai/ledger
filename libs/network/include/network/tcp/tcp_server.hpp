#ifndef NETWORK_TCP_SERVER_HPP
#define NETWORK_TCP_SERVER_HPP

#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/tcp/client_connection.hpp"
#include "network/details/thread_manager.hpp"
#include "network/tcp/client_connection.hpp"
#include "network/tcp/connection_register.hpp"

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
  typedef uint64_t handle_type;

  typedef ThreadManager thread_manager_type;
  typedef typename ThreadManager::event_handle_type event_handle_type;

  struct Request {
    handle_type handle;
    message_type meesage;
  };

  TCPServer(uint16_t const& port, thread_manager_type const& thread_manager)
      : thread_manager_(thread_manager),
        port_{port},
        request_mutex_(__LINE__, __FILE__)
  {
    LOG_STACK_TRACE_POINT;
    manager_ = std::make_shared< ClientManager >(*this);
    
      
    thread_manager_.Post([this]
    {

//      if(strongAccep && strongSocket)
//      {
//        acceptor_ = strongAccep;
//        socket_ = strongSocket;
        Accept();
//      } else {
//        std::cout << "Failed to get acceptor and socket in tcp server" << std::endl;
//      }
    });
  }

  ~TCPServer() {
    LOG_STACK_TRACE_POINT;
//    if (manager_ != nullptr) delete manager_;
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

  std::string GetAddress(handle_type const& client) {
    LOG_STACK_TRACE_POINT;
    return manager_->GetAddress(client);
  }

 private:
  thread_manager_type     thread_manager_;
  uint16_t                port_;
  //event_handle_type       event_service_start_;
  std::deque<Request>     requests_;
  fetch::mutex::Mutex     request_mutex_;

  void Accept() {
    LOG_STACK_TRACE_POINT;

    auto strongAccep = thread_manager_.CreateIO<asio::ip::tcp::tcp::acceptor>
      (asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port_));
    auto strongSocket = thread_manager_.CreateIO<asio::ip::tcp::tcp::socket>();
    
//    auto strongAccep = acceptor_.lock();    
//    auto strongSocket = socket_.lock();
//    if(!strongAccep || !strongSocket) return;

    std::weak_ptr< ClientManager >  man = manager_;
        
    auto cb = [this, man, strongAccep, strongSocket](std::error_code ec) {
      auto lock_ptr = man.lock();
      if(!lock_ptr) return;
      
      //LOG_LAMBDA_STACK_TRACE_POINT;
      if (!ec) {
        auto conn = std::make_shared<ClientConnection>(strongSocket, manager_);
        auto ptr = connection_register_.lock();

        if(ptr) {
          ptr->Enter( conn->network_client_pointer() );
        }
        
        conn->Start();
      }

      Accept();
    };

    strongAccep->async_accept(*strongSocket, cb);
  }

  // TODO: (`HUT`) : make this solid
  std::weak_ptr< ConnectionRegister > connection_register_;
//  std::weak_ptr<asio::ip::tcp::tcp::acceptor> acceptor_;
//  std::weak_ptr<asio::ip::tcp::tcp::socket>   socket_;
  std::shared_ptr< ClientManager >            manager_;

///  fetch::mutex::Mutex request_mutex_;
//  thread_manager_type thread_manager_;
//  std::shared_ptr<asio::ip::tcp::tcp::acceptor> acceptor_;
//  std::shared_ptr<asio::ip::tcp::tcp::socket> socket_;
//  std::deque<Request> requests_;
//  ClientManager* manager_;

};
}
}

#endif
