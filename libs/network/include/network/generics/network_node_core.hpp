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

class NetworkNodeCore
{
public:
  typedef std::recursive_mutex mutex_type;
  typedef std::lock_guard<mutex_type> lock_type;
  typedef fetch::service::ServiceClient<network::TCPClient> client_type;
  typedef uint32_t protocol_number_type;

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
    tm_. Start();

    rpcPort_ = rpcPort;
    rpcServer_ = std::make_shared<service::ServiceServer<fetch::network::TCPServer>>(rpcPort, tm_);

    httpServer_ = std::make_shared<fetch::http::HTTPServer>(httpPort, tm_);

    // Add middleware to the HTTP server - allow requests from any address,
    // and print requests to the terminal in colour
    httpServer_->AddMiddleware(fetch::http::middleware::AllowOrigin("*"));
    httpServer_->AddMiddleware(fetch::http::middleware::ColorLog);

    tm_. Start();
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

  virtual std::shared_ptr<client_type> ConnectTo(const std::string &host)
  {
    return ConnectToPeer(fetch::swarm::SwarmPeerLocation(host));
  }

  virtual std::shared_ptr<client_type> ConnectTo(const std::string &host, unsigned short port)
  {
    auto remote_host_identifier = std::make_pair(host, port);
    auto iter = cache_.find(remote_host_identifier);
    if (iter != cache_.end())
      {
        return iter -> second;
      }
    auto new_client_conn = ActuallyConnectTo(host, port);
    if (new_client_conn)
      {
        cache_[remote_host_identifier] = new_client_conn;
      }
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
            return std::shared_ptr<client_type> client();
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

  void AddModule(fetch::http::HTTPModule *handler)
  {
    httpServer_->AddModule(*handler);
  }

  template<class MODULE>
  void AddModule(std::shared_ptr<MODULE> module_p)
  {
    fetch::http::HTTPModule *h = module_p.get();
    AddModule(h);
  }

  template<class INTERFACE_CLASS, class PROTOCOL_CLASS>
  void AddProtocol(INTERFACE_CLASS *interface, uint32_t protocolNumber)
  {
    lock_type mlock(mutex_);
    auto protocolInstance = std::make_shared<PROTOCOL_CLASS>(interface);
    PROTOCOL_CLASS *proto_ptr = protocolInstance.get();
    fetch::service::Protocol *base_ptr = proto_ptr;
    rpcServer_ -> Add(protocolNumber, base_ptr);
    auto protocolOwner = std::shared_ptr<ProtocolOwner>(new SpecificProtocolOwner<PROTOCOL_CLASS>(protocolInstance));
    protocols_[protocolNumber] = protocolOwner;

    std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^  ADD PROTOCOL " << protocolNumber << std::endl;
  }

  template<class INTERFACE_CLASS>
  void AddProtocol(INTERFACE_CLASS *interface, uint32_t protocolNumber)
  {
    AddProtocol<INTERFACE_CLASS, typename INTERFACE_CLASS::protocol_class_type>(interface, protocolNumber);
  }

  template<class INTERFACE_CLASS>
  void AddProtocol(std::shared_ptr<INTERFACE_CLASS> interface)
  {
    auto protocolNumber = INTERFACE_CLASS::protocol_number;
    INTERFACE_CLASS *interface_ptr = interface.get();
    AddProtocol<INTERFACE_CLASS>(interface_ptr, protocolNumber);
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
