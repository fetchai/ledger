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

  enum
  {
    IDENTITY = 1,
    DIRECTORY
  };

  P2PService(certificate_type &&certificate, uint16_t port,
             fetch::network::NetworkManager const &tm)
    : super_type(port, tm), manager_(tm), certificate_(std::move(certificate))
  {
    running_     = false;
    thread_pool_ = network::MakeThreadPool(1);
    fetch::logger.Warn("Establishing P2P Service on rpc://0.0.0.0:", port);
    fetch::logger.Warn("P2P Identity: ",
                       byte_array::ToBase64(certificate_->identity().identifier()));

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

    p2p_trust_.reset(new P2PTrust<byte_array::ConstByteArray>());
    // p2p_trust_protocol_.reset(new TrustModelProtocol(*directory_)); // TODO(kll)
    // this->Add(DIRECTORY, p2p_trust_protocol_.get()); // TODO(kll)

    // Adding hooks for listening to feeds etc
    register_.OnClientEnter(
        [](connection_handle_type const &i) { fetch::logger.Info("Peer joined: ", i); });

    register_.OnClientLeave(
        [](connection_handle_type const &i) { fetch::logger.Info("Peer left: ", i); });

    // TODO(issue 7): Get from settings
    min_connections_ = 2;
    max_connections_ = 6;
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

    if (running_) return;
    running_ = true;

    NextServiceCycle();
  }

  void Stop()
  {
    if (!running_) return;
    running_ = false;

    directory_->Stop();
    thread_pool_->Stop();
  }

  client_register_type connection_register() { return register_; };

  ///
  /// @{
  void RequestPeers() { directory_->RequestPeersForThisNode(); }

  void EnoughPeers() { directory_->EnoughPeersForThisNode(); }

  P2PPeerDirectory::peer_details_map_type SuggestPeersToConnectTo()
  {
    return directory_->SuggestPeersToConnectTo();
  }
  /// @}

  bool Connect(byte_array::ConstByteArray const &host, uint16_t const &port)
  {
    fetch::logger.Info("START P2P CONNECT: Host: ", host, " port: ", port);

    shared_service_client_type client =
        register_.CreateServiceClient<client_type>(manager_, host, port);

    LOG_STACK_TRACE_POINT;
    std::size_t n = 0;
    while ((n < 10) && (!client->is_alive()))
    {
      LOG_STACK_TRACE_POINT;
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      ++n;
    }

    if (n >= 10)
    {
      fetch::logger.Error("Connection never came to live in P2P module. Host: ", host,
                          " port: ", port);
      // TODO(issue 11): throw error?
      client.reset();
      return false;
    }

    // Getting own IP seen externally
    byte_array::ByteArray address;
    auto                  p = client->Call(IDENTITY, P2PIdentityProtocol::EXCHANGE_ADDRESS, host);
    /*
      if (!p.Wait(20))
    {
      fetch::logger.Error("Connection doesn't seem to do IDENTITY");
      client.reset();
      return false;
    }
    */
    p.As(address);

    fetch::logger.Info("Looks like my address is: ", address);

    {  // Exchanging identities including node setup
      LOG_STACK_TRACE_POINT;

      // Updating IP for P2P node
      for (auto &e : my_details_->details.entry_points)
      {
        if (e.is_discovery)
        {
          // TODO(issue 25): Make mechanim for verifying address
          std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
          e.host.insert(address);
        }
      }

      auto        p = client->Call(IDENTITY, P2PIdentityProtocol::HELLO, my_details_->details);
      PeerDetails details = p.As<PeerDetails>();

      fetch::logger.Info("The remove identity is: ",
                         byte_array::ToBase64(details.identity.identifier()));

      auto regdetails = register_.GetDetails(client->handle());

      {
        LOG_STACK_TRACE_POINT;
        std::lock_guard<mutex::Mutex> lock(*regdetails);

        regdetails->Update(details);
      }

      {
        std::lock_guard<mutex_type> lock_(peers_mutex_);
        peers_[client->handle()] = client;
        peer_identities_.emplace(details.identity.identifier());
      }
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

  void PublishProfile() { identity_->PublishProfile(); }
  /// @}

  /// Methods to get profile information
  /// @{
  PeerDetails Profile() const
  {
    std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
    return my_details_->details;
  }
  /// }

  byte_array::ConstByteArray identity() const { return certificate_->identity().identifier(); }

protected:
  callback_peer_update_profile_type callback_peer_update_profile_;

  /// Service maintainance
  /// @{
  void NextServiceCycle()
  {
      connected_identities_type new_incoming;
      connected_identities_type new_outgoing;

    std::vector<EntryPoint> orchestration;
    {
      LOG_STACK_TRACE_POINT;
      if (!running_) return;

      // Updating lists of incoming and outgoing
      using map_type = client_register_type::connection_map_type;

//      register_.WithConnections([this, &orchestration](map_type const &map) {
//        for (auto &c : map)

      register_.VisitConnections([this, &orchestration, &new_incoming, &new_outgoing](map_type::value_type const &c){
          auto conn = c.second.lock();
          if (conn)
          {
            LOG_STACK_TRACE_POINT
            auto details = register_.GetDetails(conn->handle());

            std::lock_guard<mutex::Mutex> lock(*details);

            switch (conn->Type())
            {
            case network::AbstractConnection::TYPE_OUTGOING:
              {
                LOG_STACK_TRACE_POINT;
                generics::MilliTimer mt("network::AbstractConnection::TYPE_OUTGOING", 0);
              new_outgoing.insert(details->identity.identifier());
              for (auto &e : details->entry_points)
              {
                LOG_STACK_TRACE_POINT
#if 0
                char const *t = "unknown";
                if (e.is_lane)
                {
                  t = "lane ";
                }
                else if (e.is_mainchain)
                {
                  t = "disco";
                }
                else if (e.is_mainchain)
                {
                  t = "chain";
                }

                fetch::logger.Info(" - type: ", t, " port: ", e.port);
                for (auto const &h : e.host)
                {
                  fetch::logger.Info("   + host: ", h);
                }
#endif

                if (!e.was_promoted)
                {
                  orchestration.push_back(e);
                  e.was_promoted = true;
                }
              }
              }
              break;
            case network::AbstractConnection::TYPE_INCOMING:
              new_incoming.insert(details->identity.identifier());
              break;
            }
          }
      });
    }

    {
      LOG_STACK_TRACE_POINT
        std::lock_guard<mutex::Mutex> lock(maintainance_mutex_);
      incoming_.clear();
      outgoing_.clear();

      incoming_ = new_incoming;
      outgoing_ = new_outgoing;
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
      LOG_STACK_TRACE_POINT
    std::lock_guard<mutex::Mutex> lock(maintainance_mutex_);

    // Timeout to send out a new tracking signal if needed
    // TODO(issue 7): Pull from settings
    {
      std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
      double                                ms =
          double(std::chrono::duration_cast<std::chrono::milliseconds>(end - track_start_).count());
      if (ms > 5000) tracking_peers_ = false;
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
      LOG_STACK_TRACE_POINT
    std::lock_guard<mutex::Mutex> lock(maintainance_mutex_);

    if (!running_) return;
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
        if (e.identity.identifier() == my_pk) continue;

        if (e.is_discovery)
        {
          bool const is_ourself = e.identity.identifier() == certificate_->identity().identifier();

          if ((!is_ourself) && (outgoing_.find(e.identity.identifier()) == outgoing_.end()))
          {
            endpoints.push_back(e);
          }
        }
      }
    }

    std::random_shuffle(endpoints.begin(), endpoints.end());

    // Connecting to peers who need connection
    for (auto const &e : endpoints)
    {
      if (create_count == 0) break;

      //      // we must be careful not to connect to peers to whom we have already connected
      //      bool already_connected = false;
      //      {
      //        std::lock_guard<mutex::Mutex> lock_peers(peers_mutex_);
      //        already_connected = peer_identities_.find(e.identity.identifier()) !=
      //        peer_identities_.end();
      //      }

      //      if (already_connected)
      //      {
      //        logger.Info("Discarding peer because already connected...");
      //        continue;
      //      }

      thread_pool_->Post([this, e]() { this->TryConnect(e); });
      --create_count;
    }

    // Connecting to other peers if needed
    // TODO(issue 24):

    thread_pool_->Post([this]() { this->NextServiceCycle(); });
  }

  void TryConnect(EntryPoint const &e)
  {
    fetch::logger.Info("#!# Trying to connect to ", byte_array::ToBase64(e.identity.identifier()));

    if (e.identity.identifier() == "")
    {
      fetch::logger.Error("Encountered empty identifier");
      return;
    }

    for (auto &h : e.host)
    {
      if (Connect(h, e.port)) break;
    }
  }

  /// @}

private:
  using connected_identity_type = byte_array::ConstByteArray;
  using connected_identities_type = std::unordered_set<connected_identity_type, crypto::CallableFNV>;

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
  connected_identities_type                                                    peer_identities_;

  std::unique_ptr<crypto::Prover> certificate_;
  std::atomic<bool>               running_;

  mutex::Mutex                          maintainance_mutex_{__LINE__, __FILE__};
  std::atomic<uint64_t>                 min_connections_;
  std::atomic<uint64_t>                 max_connections_;
  std::atomic<bool>                     tracking_peers_;
  std::chrono::system_clock::time_point track_start_;

  
  connected_identities_type incoming_;
  connected_identities_type outgoing_;
  std::vector<connection_handle_type>                                 incoming_handles_;
  std::vector<connection_handle_type>                                 outgoing_handles_;
};

}  // namespace p2p
}  // namespace fetch
