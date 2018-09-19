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

#include <memory>

#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "network/details/thread_pool.hpp"
#include "network/management/connection_register.hpp"
#include "network/p2pservice/p2p_identity.hpp"
#include "network/p2pservice/p2p_identity_protocol.hpp"
#include "network/p2pservice/p2p_peer_details.hpp"
#include "network/p2pservice/p2p_peer_directory.hpp"
#include "network/p2pservice/p2p_peer_directory_protocol.hpp"
#include "network/p2pservice/p2ptrust.hpp"
#include "network/service/server.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace fetch {
namespace p2p {

class P2PService : public service::ServiceServer<fetch::network::TCPServer>
{
public:
  using connectivity_details_type = PeerDetails;
  using client_register_type      = fetch::network::ConnectionRegister<connectivity_details_type>;
  //  using identity_type = LaneIdentity;
  //  using identity_protocol_type = LaneIdentityProtocol;
  using connection_handle_type = client_register_type::connection_handle_type;

  using client_type = fetch::network::TCPClient;
  using super_type  = service::ServiceServer<fetch::network::TCPServer>;

  using thread_pool_type           = network::ThreadPool;
  using service_client_type        = fetch::service::ServiceClient;
  using shared_service_client_type = std::shared_ptr<service_client_type>;
  using network_manager_type       = fetch::network::NetworkManager;
  using mutex_type                 = fetch::mutex::Mutex;

  using p2p_identity_type                 = std::unique_ptr<P2PIdentity>;
  using p2p_identity_protocol_type        = std::unique_ptr<P2PIdentityProtocol>;
  using p2p_directory_type                = std::unique_ptr<P2PPeerDirectory>;
  using p2p_directory_protocol_type       = std::unique_ptr<P2PPeerDirectoryProtocol>;
  using callback_peer_update_profile_type = std::function<void(EntryPoint const &)>;
  using certificate_type                  = std::unique_ptr<crypto::Prover>;
  using p2p_trust_type                    = std::unique_ptr<P2PTrust<byte_array::ConstByteArray>>;

  static constexpr char const *LOGGING_NAME = "P2PService";

  enum
  {
    IDENTITY = 1,
    DIRECTORY
  };

  P2PService(certificate_type &&certificate, uint16_t port,
             fetch::network::NetworkManager const &tm);

  /// Events for new peer discovery
  /// @{
  void OnPeerUpdateProfile(callback_peer_update_profile_type const &f)
  {
    callback_peer_update_profile_ = f;
  }
  /// @}

  /// Methods to interact with peers
  /// @{
  void Start();
  void Stop();

  client_register_type connection_register()
  {
    return register_;
  };

  ///
  /// @{
  void RequestPeers()
  {
    directory_->RequestPeersForThisNode();
  }

  void EnoughPeers()
  {
    directory_->EnoughPeersForThisNode();
  }

  P2PPeerDirectory::peer_details_map_type SuggestPeersToConnectTo();
  /// @}

  bool Connect(byte_array::ConstByteArray const &host, uint16_t const &port);
  /// @}

  /// Methods to add node components
  /// @{
  void AddLane(uint32_t const &lane, byte_array::ConstByteArray const &host, uint16_t const &port,
               crypto::Identity const &identity = crypto::Identity());

  void AddMainChain(byte_array::ConstByteArray const &host, uint16_t const &port);

  void PublishProfile()
  {
    identity_->PublishProfile();
  }
  /// @}

  /// Methods to get profile information
  /// @{
  PeerDetails Profile() const
  {
    std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
    return my_details_->details;
  }
  /// }

  byte_array::ConstByteArray identity() const
  {
    return certificate_->identity().identifier();
  }

protected:
  callback_peer_update_profile_type callback_peer_update_profile_;

  /// Service maintainance
  /// @{
  void NextServiceCycle();
  void ManageIncomingConnections();
  void ConnectToNewPeers();
  void TryConnect(EntryPoint const &e);
  /// @}

private:
  using connected_identity_type   = byte_array::ConstByteArray;
  using connected_identities_type = std::unordered_set<connected_identity_type>;

  network_manager_type manager_;
  client_register_type register_;
  thread_pool_type     thread_pool_;

  p2p_identity_type          identity_;
  p2p_identity_protocol_type identity_protocol_;

  p2p_directory_type          directory_;
  p2p_directory_protocol_type directory_protocol_;

  p2p_trust_type p2p_trust_;
  // p2p_trust_protocol_type     p2p_trust_protocol_;  //TODO(kll)

  NodeDetails my_details_;

  mutex::Mutex peers_mutex_{__LINE__, __FILE__};
  std::unordered_map<connection_handle_type, shared_service_client_type> peers_;
  connected_identities_type                                              peer_identities_;

  std::unique_ptr<crypto::Prover> certificate_;
  std::atomic<bool>               running_;

  mutex::Mutex                          maintainance_mutex_{__LINE__, __FILE__};
  std::atomic<uint64_t>                 min_connections_;
  std::atomic<uint64_t>                 max_connections_;
  std::atomic<bool>                     tracking_peers_;
  std::chrono::system_clock::time_point track_start_;

  connected_identities_type           incoming_;
  connected_identities_type           outgoing_;
  std::vector<connection_handle_type> incoming_handles_;
  std::vector<connection_handle_type> outgoing_handles_;
};

}  // namespace p2p
}  // namespace fetch
