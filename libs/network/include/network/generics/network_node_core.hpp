#ifndef NETWORK_NODE_CORE_HPP
#define NETWORK_NODE_CORE_HPP

#include <iostream>
#include <string>
#include <stdexcept>

#include "network/service/client.hpp"
#include "network/service/server.hpp"
#include "http/server.hpp"
#include "http/middleware/allow_origin.hpp"
#include "http/middleware/color_log.hpp"

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

    std::cout << "%%%%%%%%%%%%%%%%%%%%% NetworkNodeCore 2" << std::endl;
    rpcServer_ = std::make_shared<service::ServiceServer<fetch::network::TCPServer>>(rpcPort, tm_);
    std::cout << "%%%%%%%%%%%%%%%%%%%%% NetworkNodeCore 3 " << httpPort  << std::endl;
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
  }

  virtual ~NetworkNodeCore()
  {
  }

  virtual std::shared_ptr<client_type> ConnectTo(const std::string &host, unsigned short port)
  {
    std::shared_ptr<client_type> client = std::make_shared<client_type>( host, port, tm_ );

    int waits = 25;
    while(!client->is_alive())
      {
        waits--;
        if (waits <= 0)
          {
            throw std::invalid_argument("Bad connection to " + host + ":" + std::to_string(port));
          }
        usleep(200);
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
  void AddModule(HTTP_HANDLER &handler)
  {
    httpServer_->AddModule(handler);
  }

  template<class INTERFACE_CLASS, class PROTOCOL_CLASS>
  void AddProtocol(INTERFACE_CLASS *interface, unsigned int protocolNumber)
  {
    lock_type mlock(mutex_);
    auto protocolInstance = std::make_shared<PROTOCOL_CLASS>(interface);
    rpcServer_ -> Add(protocolNumber, protocolInstance.get());
    auto protocolOwner = std::shared_ptr<ProtocolOwner>(new SpecificProtocolOwner<PROTOCOL_CLASS>(protocolInstance));
    protocols_[protocolNumber] = protocolOwner;
  }

  template<class INTERFACE_CLASS>
  void AddProtocol(INTERFACE_CLASS *interface, unsigned int protocolNumber)
  {
    lock_type mlock(mutex_);
    auto protocolInstance = std::make_shared<typename INTERFACE_CLASS::protocol_class_type>(interface);
    rpcServer_ -> Add(protocolNumber, protocolInstance.get());
    auto protocolOwner = std::shared_ptr<ProtocolOwner>(new SpecificProtocolOwner<typename INTERFACE_CLASS::protocol_class_type>(protocolInstance));
    protocols_[protocolNumber] = protocolOwner;
    interfaces_[protocolNumber] = interface;
  }

  template<class INTERFACE_CLASS>
  void AddProtocol(INTERFACE_CLASS *interface)
  {
    lock_type mlock(mutex_);
    auto protocolNumber = INTERFACE_CLASS::protocol_number;
    auto protocolInstance = std::make_shared<typename INTERFACE_CLASS::protocol_class_type>(interface);
    rpcServer_ -> Add(protocolNumber, protocolInstance.get());
    auto protocolOwner = std::shared_ptr<ProtocolOwner>(new SpecificProtocolOwner<typename INTERFACE_CLASS::protocol_class_type>(protocolInstance));
    protocols_[protocolNumber] = protocolOwner;
    interfaces_[protocolNumber] = interface;
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
  mutex_type mutex_;
  std:: map<protocol_number_type, std::shared_ptr<ProtocolOwner>> protocols_;
  std:: map<protocol_number_type, void*> interfaces_;
  std::shared_ptr<fetch::http::HTTPServer> httpServer_;
  std::shared_ptr<service::ServiceServer<fetch::network::TCPServer>> rpcServer_;

};

}
}

#endif //NETWORK_NODE_CORE_HPP
