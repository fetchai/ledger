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

#include "network/details/thread_pool.hpp"
#include "network/management/connection_register.hpp"
#include "network/p2pservice/p2p_peer_details.hpp"
#include "network/service/server.hpp"

#include "network/p2pservice/p2p_identity.hpp"
#include "network/p2pservice/p2p_identity_protocol.hpp"

#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "network/p2pservice/p2p_peer_directory.hpp"
#include "network/p2pservice/p2p_peer_directory_protocol.hpp"
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

  enum
  {
    IDENTITY = 1,
    DIRECTORY
  };

  P2PService(uint16_t port, fetch::network::NetworkManager const &tm)
    : super_type(port, tm)
    , manager_(tm)
  {
    running_     = false;
    thread_pool_ = network::MakeThreadPool(1);
    fetch::logger.Warn("Establishing P2P Service on rpc://0.0.0.0:", port);

    // TODO(issue 24): Load from somewhere
    crypto::ECDSASigner *certificate = new crypto::ECDSASigner();
    certificate->GenerateKeys();
    certificate_.reset(certificate);

    // Listening for new connections
    this->SetConnectionRegister(register_);

    // Identity
    identity_          = std::make_unique<P2PIdentity>(IDENTITY, register_, tm);
    my_details_        = identity_->my_details();
    identity_protocol_ = std::make_unique<P2PIdentityProtocol>(identity_.get());
    this->Add(IDENTITY, identity_protocol_.get());

    {
      EntryPoint discovery_ep;
      discovery_ep.identity     = certificate_->identity();
      discovery_ep.is_discovery = true;
      discovery_ep.port         = port;
      std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
      my_details_->details.entry_points.push_back(discovery_ep);
      my_details_->details.identity = certificate_->identity();

      my_details_->details.Sign(certificate_.get());
#if 0
      // TODO(issue 24): ECDSA verifier broke
      crypto::ECDSAVerifier verifier(certificate->identity());
      if(!my_details_->details.Verify(&verifier) ) {
        TODO_FAIL("Could not verify own identity");
      }
#endif
    }

    // P2P Peer Directory
    directory_ =
        std::make_unique<P2PPeerDirectory>(DIRECTORY, register_, thread_pool_, my_details_);
    directory_protocol_ = std::make_unique<P2PPeerDirectoryProtocol>(*directory_);
    this->Add(DIRECTORY, directory_protocol_.get());

    // Adding hooks for listening to feeds etc
    register_.OnClientEnter([](connection_handle_type const &i) {
      std::cout << "\rNew connection " << i << std::endl;
    });

    register_.OnClientLeave(
        [](connection_handle_type const &i) { std::cout << "\rPeer left " << i << std::endl; });

    // TODO(issue 7): Get from settings
    min_connections_ = 2;
    max_connections_ = 3;
    tracking_peers_  = false;

    // TODO(issue 24): Remove long term
    Start();
  }

  /// Events for new peer discovery
  /// @{
  void OnPeerUpdateProfile(callback_peer_update_profile_type const &f)
  {
    callback_peer_update_profile_ = f;
  }

  /// @}

  /// Methods to interact with peers
  /// @{
  void Start()
  {
    thread_pool_->Start();
    directory_->Start();

    if (running_)
    {
      return;
    }
    running_ = true;

    NextServiceCycle();
  }

  void Stop()
  {
    if (!running_)
    {
      return;
    }
    running_ = false;

    directory_->Stop();
    thread_pool_->Stop();
  }

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

  P2PPeerDirectory::peer_details_map_type SuggestPeersToConnectTo()
  {
    return directory_->SuggestPeersToConnectTo();
  }
  /// @}

  bool Connect(byte_array::ConstByteArray const &host, uint16_t const &port)
  {
    shared_service_client_type client =
        register_.CreateServiceClient<client_type>(manager_, host, port);

    std::size_t n = 0;
    while ((n < 10) && (!client->is_alive()))
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      ++n;
    }

    if (n >= 10)
    {
      fetch::logger.Error("Connection never came to live in P2P module");
      // TODO(issue 11): throw error?
      client.reset();
      return false;
    }

    // Getting own IP seen externally
    byte_array::ByteArray address;
    auto                  p = client->Call(IDENTITY, P2PIdentityProtocol::EXCHANGE_ADDRESS, host);
    p.As(address);

    {  // Exchanging identities including node setup
      std::lock_guard<mutex::Mutex> lock(my_details_->mutex);

      // Updating IP for P2P node
      for (auto &e : my_details_->details.entry_points)
      {
        if (e.is_discovery)
        {
          // TODO(issue 25): Make mechanim for verifying address
          e.host.insert(address);
        }
      }

      auto        p = client->Call(IDENTITY, P2PIdentityProtocol::HELLO, my_details_->details);
      PeerDetails details = p.As<PeerDetails>();

      auto regdetails = register_.GetDetails(client->handle());
      {
        std::lock_guard<mutex::Mutex> lock(*regdetails);
        regdetails->Update(details);
      }
    }

    {
      std::lock_guard<mutex_type> lock_(peers_mutex_);
      peers_[client->handle()] = client;
    }

    // Setup feeds to listen to
    directory_->ListenTo(client);

    return true;
  }
  /// @}

  /// Methods to add node components
  /// @{
  void AddLane(uint32_t const &lane, byte_array::ConstByteArray const &host, uint16_t const &port,
               crypto::Identity const &identity = crypto::Identity())
  {
    // TODO(issue 24): connect

    {
      std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
      EntryPoint                    lane_details;
      lane_details.host.insert(host);
      lane_details.port     = port;
      lane_details.identity = identity;
      lane_details.lane_id  = lane;
      lane_details.is_lane  = true;
      my_details_->details.entry_points.push_back(lane_details);
    }

    identity_->MarkProfileAsUpdated();
  }

  void AddMainChain(byte_array::ConstByteArray const &host, uint16_t const &port)
  {

    // TODO(issue 24): connect
    {
      std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
      EntryPoint                    mainchain_details;

      mainchain_details.host.insert(host);
      mainchain_details.port = port;
      //     lane_details.public_key = "todo";
      mainchain_details.is_mainchain = true;
      my_details_->details.entry_points.push_back(mainchain_details);
    }

    identity_->MarkProfileAsUpdated();
  }

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

protected:
  callback_peer_update_profile_type callback_peer_update_profile_;

  /// Service maintainance
  /// @{
  void NextServiceCycle()
  {
    std::vector<EntryPoint> orchestration;

    {
      std::lock_guard<mutex::Mutex> lock(maintainance_mutex_);
      if (!running_)
      {
        return;
      }

      // Updating lists of incoming and outgoing
      using map_type = client_register_type::connection_map_type;
      incoming_.clear();
      outgoing_.clear();

      register_.WithConnections([this, &orchestration](map_type const &map) {
        for (auto &c : map)
        {
          auto conn = c.second.lock();
          if (conn)
          {
            auto                          details = register_.GetDetails(conn->handle());
            std::lock_guard<mutex::Mutex> lock(*details);

            switch (conn->Type())
            {
            case network::AbstractConnection::TYPE_OUTGOING:
              outgoing_.insert(details->identity.identifier());
              for (auto &e : details->entry_points)
              {
                if (!e.was_promoted)
                {
                  orchestration.push_back(e);
                  e.was_promoted = true;
                }
              }

              break;
            case network::AbstractConnection::TYPE_INCOMING:
              incoming_.insert(details->identity.identifier());
              break;
            }
          }
        }
      });
    }

    for (auto &e : orchestration)
    {
      thread_pool_->Post([this, e]() {
        if (!e.is_discovery)
        {
          if (callback_peer_update_profile_)
          {
            callback_peer_update_profile_(e);
          }
        }
      });
    }

    thread_pool_->Post([this]() { this->ManageIncomingConnections(); },
                       1000);  // TODO(issue 7): add to config
  }

  void ManageIncomingConnections()
  {
    std::lock_guard<mutex::Mutex> lock(maintainance_mutex_);

    // Timeout to send out a new tracking signal if needed
    // TODO(issue 7): Pull from settings
    {
      std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
      double                                ms =
          double(std::chrono::duration_cast<std::chrono::milliseconds>(end - track_start_).count());
      if (ms > 5000)
      {
        tracking_peers_ = false;
      }
    }

    // Requesting new connections as needed
    if (incoming_.size() < min_connections_)
    {
      if (!tracking_peers_)
      {
        track_start_    = std::chrono::system_clock::now();
        tracking_peers_ = true;
        directory_->RequestPeersForThisNode();
      }
    }
    else if (incoming_.size() >= min_connections_)
    {
      if (tracking_peers_)
      {
        tracking_peers_ = false;
        directory_->EnoughPeersForThisNode();
      }
    }

    // Kicking random peers if needed
    if (incoming_.size() > max_connections_)
    {
      std::cout << "Too many incoming connections" << std::endl;
      // TODO(issue 24):
    }

    thread_pool_->Post([this]() { this->ConnectToNewPeers(); });
  }

  void ConnectToNewPeers()
  {
    std::lock_guard<mutex::Mutex> lock(maintainance_mutex_);

    if (!running_)
    {
      return;
    }
    uint64_t create_count = 0;

    if (outgoing_.size() < min_connections_)
    {
      create_count = min_connections_ - outgoing_.size();
    }
    else if (outgoing_.size() < max_connections_)
    {
      create_count = 1;
    }

    byte_array::ConstByteArray my_pk;
    {
      std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
      my_pk = my_details_->details.identity.identifier().Copy();
    }

    // Creating list of endpoints
    P2PPeerDirectory::peer_details_map_type suggest = directory_->SuggestPeersToConnectTo();
    std::vector<EntryPoint>                 endpoints;
    for (auto &s : suggest)
    {
      for (auto &e : s.second.entry_points)
      {
        if (e.identity.identifier() == my_pk)
        {
          continue;
        }

        if (e.is_discovery)
        {
          if (outgoing_.find(e.identity.identifier()) == outgoing_.end())
          {
            endpoints.push_back(e);
          }
        }
      }
    }
    std::random_shuffle(endpoints.begin(), endpoints.end());

    // Connecting to peers who need connection
    for (auto e : endpoints)
    {
      if (create_count == 0)
      {
        break;
      }
      thread_pool_->Post([this, e]() { this->TryConnect(e); });
      --create_count;
    }

    // Connecting to other peers if needed
    // TODO(issue 24):

    thread_pool_->Post([this]() { this->NextServiceCycle(); });
  }

  void TryConnect(EntryPoint const &e)
  {
    if (e.identity.identifier() == "")
    {
      fetch::logger.Error("Encountered empty identifier");
      return;
    }

    fetch::logger.Debug("Trying to connect to ", byte_array::ToBase64(e.identity.identifier()));
    for (auto &h : e.host)
    {
      if (Connect(h, e.port))
      {
        break;
      }
    }
  }

  /// @}

private:
  network_manager_type manager_;
  client_register_type register_;
  thread_pool_type     thread_pool_;

  p2p_identity_type          identity_;
  p2p_identity_protocol_type identity_protocol_;

  p2p_directory_type          directory_;
  p2p_directory_protocol_type directory_protocol_;

  NodeDetails my_details_;

  mutex::Mutex                                                           peers_mutex_;
  std::unordered_map<connection_handle_type, shared_service_client_type> peers_;

  std::unique_ptr<crypto::Prover> certificate_;
  std::atomic<bool>               running_;

  mutex::Mutex                          maintainance_mutex_;
  std::atomic<uint64_t>                 min_connections_;
  std::atomic<uint64_t>                 max_connections_;
  std::atomic<bool>                     tracking_peers_;
  std::chrono::system_clock::time_point track_start_;

  std::unordered_set<byte_array::ConstByteArray> incoming_;
  std::unordered_set<byte_array::ConstByteArray> outgoing_;
  std::vector<connection_handle_type>            incoming_handles_;
  std::vector<connection_handle_type>            outgoing_handles_;
};

}  // namespace p2p
}  // namespace fetch
