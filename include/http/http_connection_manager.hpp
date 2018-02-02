#ifndef HTTP_CONNECTION_MANAGER_HPP
#define HTTP_CONNECTION_MANAGER_HPP
#include"http/abstract_connection.hpp"
#include"http/abstract_server.hpp"
#include"logger.hpp"

namespace fetch {
namespace http {

class HTTPConnectionManager 
{
public:
  typedef typename AbstractHTTPConnection::shared_type connection_type;
  typedef uint64_t handle_type;

  HTTPConnectionManager(AbstractHTTPServer& server) : server_(server), clients_mutex_(__LINE__, __FILE__) {}

  handle_type Join(connection_type client) 
  {
    handle_type handle = server_.next_handle();
    fetch::logger.Info("Client joining with handle ", handle);
    
    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);
    clients_[handle] = client;
    return handle;
  }

  void Leave(handle_type handle) {    
    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);
    
    if( clients_.find(handle) != clients_.end() ) 
    {
      fetch::logger.Info("Client ", handle, " is leaving");
      // TODO: Close socket!
      clients_.erase(handle);
    }
  }

  bool Send(handle_type client, HTTPResponse const& msg) 
  {
    /*
    bool ret = true;
    clients_mutex_.lock();
    
    if( clients_.find( client ) != clients_.end() ) 
    {
      auto c = clients_[client];
      clients_mutex_.unlock();      
      c->Send(msg);
      fetch::logger.Debug("Client manager did send message to ", client);      
      clients_mutex_.lock();
    } else 
    {
      fetch::logger.Debug("Client not found.");  
      ret = false;      
    }
    clients_mutex_.unlock();
    return ret;
    */
    return false;
    
  }

  
  void PushRequest(handle_type client, HTTPRequest const& req) 
  {
    server_.PushRequest(client, req);
  }

  std::string GetAddress(handle_type client) 
  {
    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);    
    if( clients_.find(client) != clients_.end() ) 
    {
      return clients_[client]->Address();
    }
    return "0.0.0.0";    
  }
  
private:
  AbstractHTTPServer& server_;
  std::map<handle_type, connection_type> clients_;
  fetch::mutex::Mutex clients_mutex_;

};

};
};


#endif 
