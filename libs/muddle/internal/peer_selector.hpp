#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "promise_runnable.hpp"

#include "core/mutex.hpp"
#include "core/periodic_runnable.hpp"
#include "crypto/fnv.hpp"
#include "network/peer.hpp"
#include "network/uri.hpp"
#include "muddle/address.hpp"
#include "muddle/rpc/client.hpp"

#include <unordered_map>
#include <unordered_set>
#include <chrono>

namespace fetch {
namespace core {

class Reactor;

}
namespace muddle {

class PeerConnectionList;
class MuddleRegister;
class MuddleEndpoint;
class DirectMessageService;

class PeerSelector : public core::PeriodicRunnable
{
public:
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;
  using Addresses = std::unordered_set<Address>;
  using UriSet    = std::unordered_set<network::Uri>;

  struct PeerMetadata
  {
    network::Peer peer;
    bool          unreachable{false};
    Timepoint     last_attempt{Clock::now()};  // replace with connection attempts

    explicit PeerMetadata(network::Peer addr) : peer{std::move(addr)} {}
  };

  using PeerData = std::vector<PeerMetadata>;

  struct Metadata
  {
    PeerData     peer_data{};
    std::size_t  peer_index{0}; // The current peer being evaluated
    Timepoint    last_updated{Clock::now()};
  };

  using PeersInfo = std::unordered_map<Address, Metadata>;

  // Construction / Destruction
  PeerSelector(Duration const &interval, core::Reactor &reactor, MuddleRegister const &reg,
               PeerConnectionList &connections, MuddleEndpoint &endpoint);
  PeerSelector(PeerSelector const &) = delete;
  PeerSelector(PeerSelector &&) = delete;
  ~PeerSelector() override = default;

  void AddDesiredPeer(Address const &address);
  void AddDesiredPeer(Address const &address, network::Peer const &hint);
  void RemoveDesiredPeer(Address const &address);
  Addresses GetDesiredPeers() const;
  Addresses GetPendingRequests() const;
  PeersInfo GetPeerCache() const;

  // Operators
  PeerSelector &operator=(PeerSelector const &) = delete;
  PeerSelector &operator=(PeerSelector &&) = delete;

private:
  using Mutex           = mutex::Mutex;
  using PendingPromised = std::unordered_map<Address, std::shared_ptr<PromiseTask>>;

  void Periodically() override;
  void ResolveAddresses(Addresses const &addresses);
  void OnResolvedAddress(Address const &address, service::Promise const &promise);
  UriSet GenerateUriSet(Addresses const &addresses);

  core::Reactor &       reactor_;
  PeerConnectionList &  connections_;
  MuddleRegister const &register_;
  MuddleEndpoint &      endpoint_;
  rpc::Client           rpc_client_;

  mutable Mutex   lock_{__LINE__, __FILE__};
  Addresses       desired_addresses_;
  PendingPromised pending_resolutions_;
  PeersInfo       peers_info_;
};

} // namespace muddle
} // namespace fetch
