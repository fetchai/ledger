#pragma once
#include "core/byte_array/byte_array.hpp"
#include "http/abstract_connection.hpp"
#include "http/abstract_server.hpp"
#include "core/logger.hpp"

namespace fetch {
namespace http {

class HTTPConnectionManager {
 public:
  typedef typename AbstractHTTPConnection::shared_type connection_type;
  typedef uint64_t handle_type;

  HTTPConnectionManager(AbstractHTTPServer& server)
      : server_(server), clients_mutex_(__LINE__, __FILE__) {}

  handle_type Join(connection_type client) {
    LOG_STACK_TRACE_POINT;

    handle_type handle = server_.next_handle();
    fetch::logger.Debug("Client joining with handle ", handle);

    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);
    clients_[handle] = client;
    return handle;
  }

  void Leave(handle_type handle) {
    LOG_STACK_TRACE_POINT;

    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);

    if (clients_.find(handle) != clients_.end()) {
      fetch::logger.Debug("Client ", handle, " is leaving");
      // TODO: Close socket!
      clients_.erase(handle);
    }
    fetch::logger.Debug("Client ", handle, " is leaving");
  }

  bool Send(handle_type client, HTTPResponse const& res) {
    LOG_STACK_TRACE_POINT;

    bool ret = true;
    clients_mutex_.lock();

    if (clients_.find(client) != clients_.end()) {
      auto c = clients_[client];
      clients_mutex_.unlock();
      c->Send(res);
      fetch::logger.Debug("Client manager did send message to ", client);
      clients_mutex_.lock();
    } else {
      fetch::logger.Debug("Client not found.");
      ret = false;
    }
    clients_mutex_.unlock();
    return ret;
  }

  void PushRequest(handle_type client, HTTPRequest const& req) {
    LOG_STACK_TRACE_POINT;

    server_.PushRequest(client, req);
  }

  std::string GetAddress(handle_type client) {
    LOG_STACK_TRACE_POINT;

    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);
    if (clients_.find(client) != clients_.end()) {
      return clients_[client]->Address();
    }

    return "0.0.0.0";
  }

 private:
  AbstractHTTPServer& server_;
  std::map<handle_type, connection_type> clients_;
  fetch::mutex::Mutex clients_mutex_;
};
}
}

