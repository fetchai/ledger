#pragma once

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

namespace fetch {

namespace network {

class NetworkNodeCore
{
public:
  using mutex_type = std::mutex;
  using lock_type  = std::lock_guard<mutex_type>;
  // using client_type = fetch::service::ServiceClient<network::TCPClient>;
  using client_type          = fetch::service::ServiceClient;
  using rpc_server_type      = service::ServiceServer<fetch::network::TCPServer>;
  using protocol_number_type = uint32_t;

protected:
  const uint32_t MILLISECONDS_TO_WAIT_FOR_ALIVE_CONNECTION = 100;
  const uint32_t MICROSECONDS_PER_MILLISECOND              = 1000;
  const uint32_t NUMBER_OF_TIMES_TO_TEST_ALIVE_CONNECTION  = 100;

public:
  NetworkNodeCore(const NetworkNodeCore &rhs) = delete;
  NetworkNodeCore(NetworkNodeCore &&rhs)      = delete;
  NetworkNodeCore operator=(const NetworkNodeCore &rhs) = delete;
  NetworkNodeCore operator=(NetworkNodeCore &&rhs)             = delete;
  bool            operator==(const NetworkNodeCore &rhs) const = delete;
  bool            operator<(const NetworkNodeCore &rhs) const  = delete;

  NetworkNodeCore(size_t threads, uint16_t httpPort, uint16_t rpcPort)
    : NetworkNodeCore(NetworkManager{threads}, httpPort, rpcPort)
  {}

  NetworkNodeCore(NetworkManager const &networkManager, uint16_t httpPort, uint16_t rpcPort)
    : nm_(networkManager)
  {
    lock_type mlock(mutex_);

    // TODO(katie) investiaget if this can be moved to Start()
    // TODO(EJF):  Confusing now network manager is passed in (and is copy)
    nm_.Start();

    rpcPort_   = rpcPort;
    rpcServer_ = std::make_shared<rpc_server_type>(rpcPort, nm_);

    httpServer_ = std::make_shared<fetch::http::HTTPServer>(httpPort, nm_);

    // Add middleware to the HTTP server - allow requests from any address,
    // and print requests to the terminal in colour
    httpServer_->AddMiddleware(fetch::http::middleware::AllowOrigin("*"));
    httpServer_->AddMiddleware(fetch::http::middleware::ColorLog);
  }

  virtual ~NetworkNodeCore() {}

  using remote_host_identifier_type = std::pair<std::string, int>;
  using client_ptr                  = std::shared_ptr<client_type>;
  using cache_type                  = std::map<remote_host_identifier_type, client_ptr>;

  cache_type cache_;

  using protocol_ptr        = std::shared_ptr<fetch::service::Protocol>;
  using protocol_cache_type = std::map<uint32_t, protocol_ptr>;

  virtual std::shared_ptr<client_type> ConnectToPeer(const fetch::swarm::SwarmPeerLocation &peer)
  {
    return ConnectTo(peer.GetHost(), peer.GetPort());
  }

  virtual std::shared_ptr<client_type> ConnectTo(const std::string &host)
  {
    return ConnectToPeer(fetch::swarm::SwarmPeerLocation(host));
  }

  virtual client_ptr ConnectTo(const std::string &host, unsigned short port)
  {
    lock_type mlock(mutex_);
    auto      remote_host_identifier = std::make_pair(host, port);
    auto      iter                   = cache_.find(remote_host_identifier);
    if (iter != cache_.end())
    {
      if (iter->second)
      {
        return iter->second;
      }
      else
      {
        cache_.erase(remote_host_identifier);
      }
    }
    auto new_client_conn = ActuallyConnectTo(host, port);
    if (new_client_conn)
    {
      cache_[remote_host_identifier] = new_client_conn;
    }
    return new_client_conn;
  }

  void Start()
  {
    // We do nothing here. It exists only
    // for symmetry with the Stop().
  }

  void Stop() { nm_.Stop(); }

  void AddModule(fetch::http::HTTPModule *handler) { httpServer_->AddModule(*handler); }

  template <class MODULE>
  void AddModule(std::shared_ptr<MODULE> module_p)
  {
    lock_type                mlock(mutex_);
    fetch::http::HTTPModule *h = module_p.get();
    AddModule(h);
  }

  template <class INTERFACE_CLASS, class PROTOCOL_CLASS>
  void AddProtocol(INTERFACE_CLASS *interface, uint32_t protocolNumber)
  {
    lock_type mlock(mutex_);
    auto      protocolInstance = std::make_shared<PROTOCOL_CLASS>(interface);
    auto baseProtocolPtr = std::static_pointer_cast<fetch::service::Protocol>(protocolInstance);
    protocolCache_[protocolNumber]      = baseProtocolPtr;
    PROTOCOL_CLASS *          proto_ptr = protocolInstance.get();
    fetch::service::Protocol *base_ptr  = proto_ptr;
    rpcServer_->Add(protocolNumber, base_ptr);
  }

  template <class INTERFACE_CLASS>
  void AddProtocol(INTERFACE_CLASS *interface, uint32_t protocolNumber)
  {
    AddProtocol<INTERFACE_CLASS, typename INTERFACE_CLASS::protocol_class_type>(interface,
                                                                                protocolNumber);
  }

  template <class INTERFACE_CLASS>
  void AddProtocol(std::shared_ptr<INTERFACE_CLASS> interface)
  {
    auto             protocolNumber = INTERFACE_CLASS::protocol_number;
    INTERFACE_CLASS *interface_ptr  = interface.get();
    AddProtocol<INTERFACE_CLASS>(interface_ptr, protocolNumber);
  }

  template <class INTERFACE_CLASS>
  void AddProtocol(INTERFACE_CLASS *interface)
  {
    auto protocolNumber = INTERFACE_CLASS::protocol_number;
    AddProtocol<INTERFACE_CLASS>(interface, protocolNumber);
  }

  virtual void Post(std::function<void()> workload) { nm_.Post(workload); }

protected:
  virtual client_ptr ActuallyConnectTo(const std::string &host, unsigned short port)
  {
    network::TCPClient connection(nm_);
    connection.Connect(host, port);

    client_ptr client = std::make_shared<client_type>(connection, nm_);

    auto waits      = NUMBER_OF_TIMES_TO_TEST_ALIVE_CONNECTION;
    auto waitTimeUS = MILLISECONDS_TO_WAIT_FOR_ALIVE_CONNECTION * MICROSECONDS_PER_MILLISECOND /
                      NUMBER_OF_TIMES_TO_TEST_ALIVE_CONNECTION;
    while (!client->is_alive())
    {
      usleep(waitTimeUS);
      waits--;
      if (waits <= 0)
      {
        // TODO(katie) make this non throwing and return empty sharedp.
        throw std::invalid_argument(
            std::string("Timeout while connecting " + host + ":" + std::to_string(port)).c_str());
      }
    }
    return client;
  }

  fetch::network::NetworkManager nm_;
  uint16_t                       rpcPort_;
  mutex_type                     mutex_;

  std::shared_ptr<fetch::http::HTTPServer> httpServer_;
  std::shared_ptr<rpc_server_type>         rpcServer_;
  protocol_cache_type                      protocolCache_;
};

}  // namespace network
}  // namespace fetch
