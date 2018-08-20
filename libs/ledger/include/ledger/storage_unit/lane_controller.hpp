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

#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "ledger/storage_unit/lane_identity.hpp"
#include "ledger/storage_unit/lane_identity_protocol.hpp"
#include "network/management/connection_register.hpp"
#include "network/p2pservice/p2p_peer_details.hpp"
#include "network/service/client.hpp"
namespace fetch {
namespace ledger {

class LaneController
{
public:
  using connectivity_details_type  = LaneConnectivityDetails;
  using client_type                = fetch::network::TCPClient;
  using service_client_type        = fetch::service::ServiceClient;
  using shared_service_client_type = std::shared_ptr<service_client_type>;
  using weak_service_client_type   = std::shared_ptr<service_client_type>;
  using client_register_type       = fetch::network::ConnectionRegister<connectivity_details_type>;
  using network_manager_type       = fetch::network::NetworkManager;
  using mutex_type                 = fetch::mutex::Mutex;
  using connection_handle_type     = client_register_type::connection_handle_type;
  using protocol_handler_type      = service::protocol_handler_type;

  LaneController(protocol_handler_type const &lane_identity_protocol,
                 std::weak_ptr<LaneIdentity> identity, client_register_type reg,
                 network_manager_type const &nm)
    : lane_identity_protocol_(lane_identity_protocol)
    , lane_identity_(std::move(identity))
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
      fetch::logger.Debug("Lane trying to connect to ", h, ":", ep.port);
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

    auto ident = lane_identity_.lock();
    if (!ident)
    {
      // TODO(issue 7): Throw exception
      TODO_FAIL("Identity lost");
    }

    // Waiting for connection to be open
    std::size_t n = 0;
    while (n < 10)
    {
      auto p = client->Call(lane_identity_protocol_, LaneIdentityProtocol::PING);
      if (p.Wait(100, false))
      {
        if (p.As<LaneIdentity::ping_type>() != LaneIdentity::PING_MAGIC)
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

    crypto::Identity peer_identity;
    {
      auto ptr = lane_identity_.lock();
      if (!ptr)
      {
        logger.Warn("Lane identity not valid!");
        client->Close();
        client.reset();
        return nullptr;
      }

      auto p = client->Call(lane_identity_protocol_, LaneIdentityProtocol::HELLO, ptr->Identity());
      if (!p.Wait(1000))  // TODO(issue 7): Make timeout configurable
      {
        logger.Warn("Connection timed out - closing");
        client->Close();
        client.reset();
        return nullptr;
      }

      p.As(peer_identity);
    }

    // Exchaning info
    auto p = client->Call(lane_identity_protocol_, LaneIdentityProtocol::GET_LANE_NUMBER);
    p.Wait(1000);  // TODO(issue 7): Make timeout configurable
    if (p.As<LaneIdentity::lane_type>() != ident->GetLaneNumber())
    {
      logger.Error("Could not connect to lane with different lane number: ",
                   p.As<LaneIdentity::lane_type>(), " vs ", ident->GetLaneNumber());
      client->Close();
      client.reset();
      // TODO(issue 11): Throw exception
      return nullptr;
    }

    {
      std::lock_guard<mutex_type> lock_(services_mutex_);
      services_[client->handle()] = client;
    }

    // Setting up details such that the rest of the lane what kind of
    // connection we are dealing with.
    auto details = register_.GetDetails(client->handle());

    details->is_outgoing = true;
    details->is_peer     = true;
    details->identity    = peer_identity;

    return client;
  }

  /// @}
private:
  protocol_handler_type       lane_identity_protocol_;
  std::weak_ptr<LaneIdentity> lane_identity_;
  client_register_type        register_;
  network_manager_type        manager_;

  mutex::Mutex                                                           services_mutex_;
  std::unordered_map<connection_handle_type, shared_service_client_type> services_;
  std::vector<connection_handle_type>                                    inactive_services_;
};

}  // namespace ledger
}  // namespace fetch
