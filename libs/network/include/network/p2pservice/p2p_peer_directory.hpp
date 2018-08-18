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

#include "core/mutex.hpp"
#include "crypto/fnv.hpp"
#include "network/management/connection_register.hpp"
#include "network/p2pservice/p2p_peer_details.hpp"
#include "network/service/client.hpp"
#include "network/service/function.hpp"
#include "network/service/publication_feed.hpp"

namespace fetch {
namespace p2p {

class P2PPeerDirectory : public fetch::service::HasPublicationFeed
{
public:
  using connectivity_details_type  = PeerDetails;
  using client_type                = fetch::network::TCPClient;
  using service_client_type        = fetch::service::ServiceClient;
  using shared_service_client_type = std::shared_ptr<service_client_type>;
  using weak_service_client_type   = std::shared_ptr<service_client_type>;
  using client_register_type       = fetch::network::ConnectionRegister<connectivity_details_type>;
  using network_manager_type       = fetch::network::NetworkManager;
  using mutex_type                 = fetch::mutex::Mutex;
  using connection_handle_type     = client_register_type::connection_handle_type;
  using protocol_handler_type      = service::protocol_handler_type;

  using peer_details_map_type = std::unordered_map<byte_array::ConstByteArray,
                                                   connectivity_details_type, crypto::CallableFNV>;
  using thread_pool_type      = network::ThreadPool;

  enum
  {
    SUGGEST_PEERS = 1,
    NEED_CONNECTIONS,
    ENOUGH_CONNECTIONS
  };

  enum
  {
    FEED_ENOUGH_CONNECTIONS = 1,
    FEED_REQUEST_CONNECTIONS,
    FEED_ANNOUNCE_PEER
  };

  P2PPeerDirectory(uint64_t const &protocol, client_register_type reg, thread_pool_type pool,
                   NodeDetails my_details)
    : protocol_(protocol)
    , register_(std::move(reg))
    , thread_pool_(std::move(pool))
    , my_details_(std::move(my_details))
  {

    running_ = false;
  }

  /// Methods to update the state of this node
  /// @{
  void RequestPeersForThisNode()
  {
    // TODO(issue 26): comment/make this clear
    register_.WithServices(
        [this](network::AbstractConnectionRegister::service_map_type const &map) {
          for (auto const &p : map)
          {
            auto wptr = p.second;
            auto peer = wptr.lock();
            if (peer)
            {
              peer->Call(protocol_, NEED_CONNECTIONS);
            }
          }
        });

    std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
    AddPeerToSuggested(my_details_->details);
  }

  void EnoughPeersForThisNode()
  {
    register_.WithServices(
        [this](network::AbstractConnectionRegister::service_map_type const &map) {
          for (auto const &p : map)
          {
            auto wptr = p.second;
            auto peer = wptr.lock();
            if (peer)
            {
              peer->Call(protocol_, ENOUGH_CONNECTIONS);
            }
          }
        });

    std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
    RemovePeerFromSuggested(my_details_->details.identity.identifier());
  }

  /// @}

  /// RPC calls
  /// @{
  void NeedConnections(connection_handle_type const &client_id)
  {
    auto details = register_.GetDetails(client_id);

    {
      std::lock_guard<mutex::Mutex> lock(*details);

      AddPeerToSuggested(*details);
    }
  }

  void EnoughConnections(connection_handle_type const &client_id)
  {
    auto details = register_.GetDetails(client_id);

    {
      std::lock_guard<mutex::Mutex> lock(*details);

      RemovePeerFromSuggested(details->identity.identifier());
    }
  }

  peer_details_map_type SuggestPeersToConnectTo()
  {
    std::lock_guard<mutex::Mutex> lock(suggest_mutex_);
    return suggested_peers_;
  }
  /// @}

  /// Maintainance logic
  /// Methods to ensure that we get info from new peers
  /// @{
  void ListenTo(std::shared_ptr<service::ServiceClient> const &client)
  {
    // TODO(issue 24): Refactor subscribe such that there is no memory leak

    client->Subscribe(protocol_, FEED_REQUEST_CONNECTIONS,
                      new service::Function<void(PeerDetails)>([this](PeerDetails const &details) {
                        this->AddPeerToSuggested(details);
                      }));

    client->Subscribe(protocol_, FEED_ENOUGH_CONNECTIONS,
                      new service::Function<void(byte_array::ConstByteArray)>(
                          [this](byte_array::ConstByteArray const &public_key) {
                            this->RemovePeerFromSuggested(public_key);
                          }));

    /*
    // TODO(issue 24): Work out whether we want this
    ptr->Subscribe(protocol_, FEED_ANNOUNCE_PEER,
    new service::Function<void(PeerDetails)>(
    [this](PeerDetails const &details) {
    this->AnnouncePeer(details);
    }));
    */
  }

  void Start()
  {
    if (running_) return;

    running_ = true;
    NextMaintainanceCycle();
  }

  void Stop() { running_ = false; }

  void NextMaintainanceCycle()
  {
    if (!running_) return;

    thread_pool_->Post([this]() { this->PruneSuggestions(); },
                       1000);  // TODO(issue 7): add to config
  }

  void PruneSuggestions()
  {
    if (!running_) return;

    std::unordered_set<byte_array::ConstByteArray, crypto::CallableFNV> to_delete;

    {
      std::lock_guard<mutex::Mutex> lock(suggest_mutex_);
      for (auto &suggestion : suggested_peers_)
      {
        auto ms = suggestion.second.MillisecondsSinceUpdate();

        if (ms > 30000)
        {  // TODO(issue 7): Make variable, add to config
          to_delete.insert(suggestion.first);
        }
      }
    }

    for (auto &key : to_delete)
    {
      RemovePeerFromSuggested(key, false);
    }

    NextMaintainanceCycle();
  }
  /// @}

private:
  /// Internals for updating the register
  /// @{
  bool AddPeerToSuggested(connectivity_details_type const &details, bool const &propagate = true)
  {
    std::lock_guard<mutex::Mutex> lock(suggest_mutex_);
    auto                          it  = suggested_peers_.find(details.identity.identifier());
    bool                          ret = false;

    if (it == suggested_peers_.end())
    {
      suggested_peers_[details.identity.identifier()] = details;
      if (propagate) this->Publish(FEED_REQUEST_CONNECTIONS, details);
      ret = true;
    }
    else
    {

      // We do not allow conseq updates unless separated by substatial tume
      if (it->second.MillisecondsSinceUpdate() > 5000)
      {  // TODO(issue 7): Config variable
        suggested_peers_[details.identity.identifier()] = details;
        if (propagate) this->Publish(FEED_REQUEST_CONNECTIONS, details);
        ret = true;
      }
    }

    return ret;
  }

  bool RemovePeerFromSuggested(byte_array::ConstByteArray const &public_key,
                               bool const &                      propagate = true)
  {
    std::lock_guard<mutex::Mutex> lock(suggest_mutex_);
    auto                          it  = suggested_peers_.find(public_key);
    bool                          ret = false;

    if (it != suggested_peers_.end())
    {
      suggested_peers_.erase(it);
      if (propagate) this->Publish(FEED_ENOUGH_CONNECTIONS, public_key);
      ret = true;
    }

    return ret;
  }
  /// @}

  std::atomic<uint64_t> protocol_;

  mutable mutex::Mutex  suggest_mutex_{__LINE__, __FILE__};
  peer_details_map_type suggested_peers_;

  std::atomic<bool>    running_;
  client_register_type register_;
  thread_pool_type     thread_pool_;
  NodeDetails          my_details_;
};

}  // namespace p2p
}  // namespace fetch
