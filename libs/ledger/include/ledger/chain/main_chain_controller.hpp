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

#include <utility>

#include "ledger/chain/main_chain_details.hpp"
#include "ledger/chain/main_chain_identity.hpp"
#include "ledger/chain/main_chain_identity_protocol.hpp"
#include "network/management/connection_register.hpp"
#include "network/p2pservice/p2p_peer_details.hpp"
#include "network/service/client.hpp"

namespace fetch {
namespace chain {

class MainChainController
{
public:
  using connectivity_details_type  = MainChainDetails;
  using client_type                = fetch::network::TCPClient;
  using service_client_type        = fetch::service::ServiceClient;
  using shared_service_client_type = std::shared_ptr<service_client_type>;
  using client_register_type       = fetch::network::ConnectionRegister<connectivity_details_type>;
  using network_manager_type       = fetch::network::NetworkManager;
  using mutex_type                 = fetch::mutex::Mutex;
  using connection_handle_type     = client_register_type::connection_handle_type;
  using protocol_handler_type      = service::protocol_handler_type;

  MainChainController(protocol_handler_type const &    identity_protocol,
                      std::weak_ptr<MainChainIdentity> identity, client_register_type reg,
                      network_manager_type const &nm)
    : identity_protocol_(identity_protocol)
    , identity_(std::move(identity))
    , register_(std::move(reg))
    , manager_(nm)
  {}

  /// External controls
  /// @{
  void RPCConnect(byte_array::ByteArray const &host, uint16_t const &port) { Connect(host, port); }

  void TryConnect(p2p::EntryPoint const &ep)
  {

    for (auto &h : ep.host)
    {
      fetch::logger.Debug("Mainchain trying to connect to ", h, ":", ep.port);
      if (Connect(h, ep.port)) break;
    }
  }

  void Shutdown() { TODO_FAIL("Needs to be implemented"); }

  void StartSync() { TODO_FAIL("Needs to be implemented"); }

  void StopSync() { TODO_FAIL("Needs to be implemented"); }

  int IncomingPeers()
  {
    std::lock_guard<mutex_type> lock_(services_mutex_);
    int                         incoming = 0;
    for (auto &peer : services_)
    {
      auto details = register_.GetDetails(peer.first);
      {
        if (details->is_peer && (!details->is_outgoing))
        {
          ++incoming;
        }
      }
    }
    return incoming;
  }

  int OutgoingPeers()
  {
    std::lock_guard<mutex_type> lock_(services_mutex_);
    int                         outgoing = 0;
    for (auto &peer : services_)
    {
      auto details = register_.GetDetails(peer.first);
      {
        if (details->is_peer && details->is_outgoing)
        {
          ++outgoing;
        }
      }
    }
    return outgoing;
  }

  /// @}

  /// Internal controls
  /// @{
  shared_service_client_type GetClient(connection_handle_type const &n)
  {
    std::lock_guard<mutex_type> lock_(services_mutex_);
    return services_[n];
  }

  shared_service_client_type Connect(byte_array::ByteArray const &host, uint16_t const &port)
  {
    shared_service_client_type client =
        register_.CreateServiceClient<client_type>(manager_, host, port);

    auto ident = identity_.lock();
    if (!ident)
    {
      // TODO(issue 11): Throw exception
      TODO_FAIL("Identity lost");
    }

    // Waiting for connection to be open
    std::size_t n = 0;
    while (n < 10)
    {
      auto p = client->Call(identity_protocol_, MainChainIdentityProtocol::PING);
      if (p.Wait(100, false))
      {
        if (p.As<MainChainIdentity::ping_type>() != MainChainIdentity::PING_MAGIC)
        {
          n = 10;
        }
        break;
      }
      ++n;
    }

    if (n >= 10)
    {
      logger.Warn("Connection timed out - closing");
      client->Close();
      client.reset();
      return nullptr;
    }

    // Exchaning info
    {
      std::lock_guard<mutex_type> lock_(services_mutex_);
      services_[client->handle()] = client;
    }

    // Setting up details such that the rest of the mainchain what kind of
    // connection we are dealing with.
    auto details = register_.GetDetails(client->handle());

    details->is_outgoing = true;
    details->is_peer     = true;

    return client;
  }

  /// @}
private:
  protocol_handler_type            identity_protocol_;
  std::weak_ptr<MainChainIdentity> identity_;
  client_register_type             register_;
  network_manager_type             manager_;

  mutex::Mutex                                                           services_mutex_;
  std::unordered_map<connection_handle_type, shared_service_client_type> services_;
  std::vector<connection_handle_type>                                    inactive_services_;
};

}  // namespace chain
}  // namespace fetch
