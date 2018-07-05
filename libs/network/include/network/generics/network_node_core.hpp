#ifndef NETWORK_NODE_CORE_HPP
#define NETWORK_NODE_CORE_HPP

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "http/middleware/allow_origin.hpp"
#include "http/middleware/color_log.hpp"
#include "http/server.hpp"
#include "network/service/client.hpp"
#include "network/service/server.hpp"
#include "network/swarm/swarm_peer_location.hpp"

namespace fetch
{


namespace network
{

class NetworkNodeCoreBaseException : public std::exception
{
public:
  NetworkNodeCoreBaseException()
  {
  }
};

  class NetworkNodeCoreCannotReachException : public NetworkNodeCoreBaseException
  {
  public:
    std::string host_;
    int port_;
    std::string msg_;
    
    NetworkNodeCoreCannotReachException(const std::string &host, int port):
      NetworkNodeCoreBaseException()
    {
      this -> host_ = host;
      this -> port_ = port;
      this -> msg_ = "cannot reach " + host + std::to_string(port);
    }
    
    virtual const char *what() const _NOEXCEPT
    {
      return this -> msg_.c_str();
    }
  };

  class NetworkNodeCoreRefusingSolipsism : public NetworkNodeCoreBaseException
  {
  public:
    NetworkNodeCoreRefusingSolipsism() :
      NetworkNodeCoreBaseException()
    {
    }
    
    virtual const char *what() const _NOEXCEPT
    {
      return "Refusing to talk to myself.";
    }
  };
  
  class NetworkNodeCoreTimeOut : public NetworkNodeCoreBaseException
  {
  public:
    std::string where_;
    NetworkNodeCoreTimeOut(const std::string &where) :
      NetworkNodeCoreBaseException()
    {
      where_ = std::string("Timeout:") +where;
    }
    
    virtual const char *what() const _NOEXCEPT
    {
      return where_.c_str();
    }
  };

class NetworkNodeCore
{
public:
  typedef std::recursive_mutex mutex_type;
  typedef std::lock_guard<mutex_type> lock_type;
  typedef fetch::service::ServiceClient<network::TCPClient> client_type;
  typedef unsigned int protocol_number_type;

public:
  NetworkNodeCore(const NetworkNodeCore &rhs)           = delete;
  NetworkNodeCore(NetworkNodeCore &&rhs)           = delete;
  NetworkNodeCore operator=(const NetworkNodeCore &rhs)  = delete;
  NetworkNodeCore operator=(NetworkNodeCore &&rhs) = delete;
  bool operator==(const NetworkNodeCore &rhs) const = delete;
  bool operator<(const NetworkNodeCore &rhs) const = delete;

  explicit NetworkNodeCore(
                           size_t threads,
                           uint16_t httpPort,
                           uint16_t rpcPort
                           )
    :
    tm_(threads)
  {
    std::cout << "%%%%%%%%%%%%%%%%%%%%% NetworkNodeCore 1" << std::endl;

    tm_. Start();

    std::cout << "%%%%%%%%%%%%%%%%%%%%% NetworkNodeCore 2 -- " << rpcPort << std::endl;

    rpcPort_ = rpcPort;
    
    rpcServer_ = std::make_shared<service::ServiceServer<fetch::network::TCPServer>>(rpcPort, tm_);
    std::cout << "%%%%%%%%%%%%%%%%%%%%% NetworkNodeCore 3 " << rpcPort  << std::endl;
    httpServer_ = std::make_shared<fetch::http::HTTPServer>(httpPort, tm_);
    std::cout << "%%%%%%%%%%%%%%%%%%%%% NetworkNodeCore 4" << std::endl;

    // Add middleware to the HTTP server - allow requests from any address,
    // and print requests to the terminal in colour
    std::cout << "%%%%%%%%%%%%%%%%%%%%% NetworkNodeCore 5" << std::endl;
    httpServer_->AddMiddleware(fetch::http::middleware::AllowOrigin("*"));
    std::cout << "%%%%%%%%%%%%%%%%%%%%% NetworkNodeCore 6" << std::endl;
    httpServer_->AddMiddleware(fetch::http::middleware::ColorLog);
    std::cout << "%%%%%%%%%%%%%%%%%%%%% NetworkNodeCore 7" << std::endl;

    tm_. Start();
    std::cout << "%%%%%%%%%%%%%%%%%%%%% NetworkNodeCore 8" << std::endl;
  }

  virtual ~NetworkNodeCore()
  {
  }

  typedef std::pair<std::string, int> remote_host_identifier_type;
  typedef std::map<remote_host_identifier_type, std::shared_ptr<client_type>> cache_type;

  cache_type cache_;

  virtual std::shared_ptr<client_type> ConnectToPeer(const fetch::swarm::SwarmPeerLocation &peer)
  {
    return ConnectTo(peer.GetHost(), peer.GetPort());
  }

  virtual std::shared_ptr<client_type> ConnectTo(const std::string &host, unsigned short port)
  {

    if (port == rpcPort_)
      {
        throw NetworkNodeCoreRefusingSolipsism();
      }

    auto remote_host_identifier = std::make_pair(host, port);
    auto iter = cache_.find(remote_host_identifier);
    if (iter != cache_.end())
      {
        return iter -> second;
      }
    auto new_client_conn = ActuallyConnectTo(host, port);
    //cache_[remote_host_identifier] = new_client_conn;
    return new_client_conn;
  }

  virtual std::shared_ptr<client_type> ActuallyConnectTo(const std::string &host, unsigned short port)
  {
    std::shared_ptr<client_type> client = std::make_shared<client_type>( host, port, tm_ );

    int waits = 100;
    while(!client->is_alive())
      {
        waits--;
        if (waits <= 0)
          {
            throw NetworkNodeCoreCannotReachException(host, port);
          }
        usleep(100);
      }
    return client;
  }

  void Start()
  {
    //tm_ . Start();
    // do nothing, already started in SEEKRIT
  }

  void Stop()
  {
    tm_ . Stop();
  }

  class ProtocolOwner
  {
  public:
    ProtocolOwner()
    {
    }
    virtual ~ProtocolOwner()
    {
    }
  };

  template<class SPECIFIC_PROTOCOL>
  class SpecificProtocolOwner : public ProtocolOwner
  {
  public:
    std::shared_ptr<SPECIFIC_PROTOCOL> proto_;
    SpecificProtocolOwner(std::shared_ptr<SPECIFIC_PROTOCOL> protoToHold_):proto_(protoToHold_)
    {
    }
    virtual ~SpecificProtocolOwner()
    {
    }
  };

  template<class HTTP_HANDLER>
  void AddModule(std::shared_ptr<HTTP_HANDLER> handler)
  {
    httpServer_->AddModule(*(handler.get()));
  }

  template<class INTERFACE_CLASS, class PROTOCOL_CLASS>
  void AddProtocol(INTERFACE_CLASS *interface, unsigned int protocolNumber)
  {
    lock_type mlock(mutex_);
    auto protocolInstance = std::make_shared<PROTOCOL_CLASS>(interface);
    rpcServer_ -> Add(protocolNumber, protocolInstance.get());
    auto protocolOwner = std::shared_ptr<ProtocolOwner>(new SpecificProtocolOwner<PROTOCOL_CLASS>(protocolInstance));
    protocols_[protocolNumber] = protocolOwner;

    std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^  ADD PROTOCOL " << protocolNumber << std::endl;
  }

  template<class INTERFACE_CLASS>
  void AddProtocol(INTERFACE_CLASS *interface, unsigned int protocolNumber)
  {
    AddProtocol<INTERFACE_CLASS, typename INTERFACE_CLASS::protocol_class_type>(interface, protocolNumber);
  }

  template<class INTERFACE_CLASS>
  void AddProtocol(INTERFACE_CLASS *interface)
  {
    auto protocolNumber = INTERFACE_CLASS::protocol_number;
    AddProtocol<INTERFACE_CLASS>(interface, protocolNumber);
  }

  template<class INTERFACE_CLASS>
  INTERFACE_CLASS *GetProtocol()
  {
    // Nasty. TODO(katie) Make this not suck later.
    auto protocolNumber = INTERFACE_CLASS::protocol_number;
    return (INTERFACE_CLASS*)(interfaces_[protocolNumber]);
  }

  virtual void Post(std::function<void ()> workload, int foo)
  {
    tm_ . Post(workload);
  }

protected:
  fetch::network::ThreadManager tm_;
  uint16_t rpcPort_;
  mutex_type mutex_;
  std:: map<protocol_number_type, std::shared_ptr<ProtocolOwner>> protocols_;
  std:: map<protocol_number_type, void*> interfaces_;
  std::shared_ptr<fetch::http::HTTPServer> httpServer_;
  std::shared_ptr<service::ServiceServer<fetch::network::TCPServer>> rpcServer_;

};

}
}

#endif //NETWORK_NODE_CORE_HPP
