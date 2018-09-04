#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

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

  void Tick()
  {
    node_->Tick();
  }

  void Tock()
  {
    node_->Tock();
  }

  void Start()
  {
    network_manager_.Start();
  }

  void Stop()
  {
    network_manager_.Stop();
  }

private:
  fetch::network::NetworkManager network_manager_;

  fetch::service::ServiceServer<fetch::network::TCPServer> service_;
  AEAProtocol *                                            aea_;
  NodeToNodeProtocol *                                     node_;
};
