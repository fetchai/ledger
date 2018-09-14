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
#include "network/uri.hpp"
namespace fetch {
namespace ledger {

class LaneController
{
public:
  using connectivity_details_type  = LaneConnectivityDetails;
  using Uri                        = fetch::network::Uri;
  using client_type                = fetch::network::TCPClient;
  using service_client_type        = fetch::service::ServiceClient;
  using shared_service_client_type = std::shared_ptr<service_client_type>;
  using weak_service_client_type   = std::shared_ptr<service_client_type>;
  using client_register_type       = fetch::network::ConnectionRegister<connectivity_details_type>;
  using network_manager_type       = fetch::network::NetworkManager;
  using mutex_type                 = fetch::mutex::Mutex;
  using connection_handle_type     = client_register_type::connection_handle_type;
  using protocol_handler_type      = service::protocol_handler_type;

  static constexpr char const *LOGGING_NAME = "LaneController";

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
      FETCH_LOG_INFO(LOGGING_NAME,"Lane trying to connect to ", h, ":", ep.port);

      if (Connect(h, ep.port)) break;
    }
  }

  void RPCConnectToURIs(const std::vector<Uri> &uris)
  {
    for(auto &thing : uris)
    {
      FETCH_LOG_INFO(LOGGING_NAME,"WILL ATTEMPT TO CONNECT TO: ", thing.ToString());
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

  void UseThesePeers(std::set<Uri> uris)
  {
    std::lock_guard<mutex_type> lock_(services_mutex_);
    auto ident = lane_identity_.lock();
    if (ident)
    {
      for(auto& uri : uris)
      {
        FETCH_LOG_INFO(LOGGING_NAME, ident -> GetLaneNumber(), " -- UseThesePeers: ", uri.ToString());
      }


      std::unordered_set<Uri> remove;
      std::unordered_set<Uri> create;

      for(auto &peer_conn : peer_connections_)
      {
        if (uris.find(peer_conn . first) == uris.end())
        {
          remove.insert(peer_conn . first);
        }
      }

      for(auto &uri : uris)
      {
        if (peer_connections_.find(uri) == peer_connections_.end())
        {
          create.insert(uri);
        }
      }

      for(auto &r : remove)
      {
        peer_connections_[r] -> Close();
        peer_connections_.erase(r);
      }

      for(auto &c : create)
      {
        auto conn = Connect(c);
        if (conn)
        {
          peer_connections_[c] = conn;
        }
      }
    }
  }

  shared_service_client_type Connect(const Uri &uri)
  {
    byte_array::ByteArray b;
    uint16_t port;
    if (uri.ParseTCP(b, port))
    {
      return Connect(b, port);
    }
    return shared_service_client_type();
  }

  shared_service_client_type Connect(byte_array::ByteArray const &host, uint16_t const &port)
  {
    FETCH_LOG_INFO(LOGGING_NAME,"Connecting to lane ", host, ":", port);

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
      FETCH_LOG_INFO(LOGGING_NAME,"Trying to ping lane service");

      auto p = client->Call(lane_identity_protocol_, LaneIdentityProtocol::PING);

      FETCH_LOG_PROMISE();
      if (p->Wait(1000, false))
      {
        if (p->As<LaneIdentity::ping_type>() != LaneIdentity::PING_MAGIC)
        {
          n = 10;
        }
        break;
      }
      ++n;
    }

    if (n >= 10)
    {
      FETCH_LOG_WARN(LOGGING_NAME,"Connection timed out - closing in LaneController::Connect:1:");
      client->Close();
      client.reset();
      return nullptr;
    }

    crypto::Identity peer_identity;
    {
      auto ptr = lane_identity_.lock();
      if (!ptr)
      {
        FETCH_LOG_WARN(LOGGING_NAME,"Lane identity not valid!");
        client->Close();
        client.reset();
        return nullptr;
      }

      auto p = client->Call(lane_identity_protocol_, LaneIdentityProtocol::HELLO, ptr->Identity());

      FETCH_LOG_PROMISE();
      if (!p->Wait(1000, true))  // TODO(issue 7): Make timeout configurable
      {
        FETCH_LOG_WARN(LOGGING_NAME,"Connection timed out - closing in LaneController::Connect:2:");
        client->Close();
        client.reset();
        return nullptr;
      }

      p->As(peer_identity);
    }

    // Exchaning info
    auto p = client->Call(lane_identity_protocol_, LaneIdentityProtocol::GET_LANE_NUMBER);

    FETCH_LOG_PROMISE();
    p->Wait(1000, true);  // TODO(issue 7): Make timeout configurable
    if (p->As<LaneIdentity::lane_type>() != ident->GetLaneNumber())
    {
      FETCH_LOG_ERROR(LOGGING_NAME,"Could not connect to lane with different lane number: ",
                   p->As<LaneIdentity::lane_type>(), " vs ", ident->GetLaneNumber());
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

    FETCH_LOG_INFO(LOGGING_NAME,"Remote identity: ", byte_array::ToBase64(peer_identity.identifier()));

    return client;
  }

  /// @}
private:
  protocol_handler_type       lane_identity_protocol_;
  std::weak_ptr<LaneIdentity> lane_identity_;
  client_register_type        register_;
  network_manager_type        manager_;

  mutex::Mutex services_mutex_{__LINE__, __FILE__};
  std::unordered_map<connection_handle_type, shared_service_client_type> services_;
  std::vector<connection_handle_type>                                    inactive_services_;

  std::map<Uri, shared_service_client_type> peer_connections_;
};

}  // namespace ledger
}  // namespace fetch
