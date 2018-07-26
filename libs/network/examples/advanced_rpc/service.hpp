#pragma once

#include "aea_protocol.hpp"
#include "node_protocol.hpp"

class FetchService
{
public:
  FetchService(uint16_t port, std::string const &info)
      : network_manager_(8), service_(port, network_manager_)
  {

    aea_  = new AEAProtocol(info);
    node_ = new NodeToNodeProtocol(network_manager_);

    aea_->set_node(node_);

    service_.Add(FetchProtocols::AEA_PROTOCOL, aea_);
    service_.Add(FetchProtocols::PEER_TO_PEER, node_);
  }

  ~FetchService()
  {
    delete aea_;
    delete node_;
  }

  void Tick() { node_->Tick(); }

  void Tock() { node_->Tock(); }

  void Start() { network_manager_.Start(); }

  void Stop() { network_manager_.Stop(); }

private:
  fetch::network::NetworkManager network_manager_;

  fetch::service::ServiceServer<fetch::network::TCPServer> service_;
  AEAProtocol *                                            aea_;
  NodeToNodeProtocol *                                     node_;
};
