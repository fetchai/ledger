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
#include "core/random/lcg.hpp"
#include "crypto/fnv.hpp"
#include "moment/deadline_timer.hpp"
#include "muddle/address.hpp"
#include "muddle/peer_selection_mode.hpp"
#include "muddle/rpc/client.hpp"
#include "network/peer.hpp"
#include "network/uri.hpp"

#include <chrono>
#include <list>
#include <unordered_map>
#include <unordered_set>

namespace fetch {
namespace core {

class Reactor;
}
namespace muddle {

class PeerConnectionList;
class MuddleRegister;
class MuddleEndpoint;
class DirectMessageService;
class NetworkId;

class PeerSelector : public core::PeriodicRunnable
{
public:
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;
  using Addresses = std::unordered_set<Address>;
  using UriSet    = std::unordered_set<network::Uri>;
  using Peers     = std::vector<network::Peer>;

  struct PeerMetadata
  {
    network::Peer peer;
    bool          unreachable{false};

    explicit PeerMetadata(network::Peer addr)
      : peer{std::move(addr)}
    {}
  };

  using PeerData = std::vector<PeerMetadata>;

  struct Metadata
  {
    PeerData    peer_data{};
    std::size_t peer_index{0};  // The current peer being evaluated
    std::size_t consecutive_failures{0};
  };

  using PeersInfo = std::unordered_map<Address, Metadata>;

  // Construction / Destruction
  PeerSelector(NetworkId const &network, Duration const &interval, core::Reactor &reactor,
               MuddleRegister const &reg, PeerConnectionList &connections,
               MuddleEndpoint &endpoint);
  PeerSelector(PeerSelector const &) = delete;
  PeerSelector(PeerSelector &&)      = delete;
  ~PeerSelector() override           = default;

  void              AddDesiredPeer(Address const &address);
  void              AddDesiredPeer(Address const &address, network::Peer const &hint);
  void              RemoveDesiredPeer(Address const &address);
  Addresses         GetDesiredPeers() const;
  Addresses         GetKademliaPeers() const;
  Addresses         GetPendingRequests() const;
  PeersInfo         GetPeerCache() const;
  PeerSelectionMode GetMode() const;
  void              SetMode(PeerSelectionMode mode);

  void UpdatePeers(Peers peers);

  // Operators
  PeerSelector &operator=(PeerSelector const &) = delete;
  PeerSelector &operator=(PeerSelector &&) = delete;

private:
  static constexpr char const *CLOCK_NAME = "PeerSelectorClock";

  using PendingPromised = std::unordered_map<Address, std::shared_ptr<PromiseTask>>;
  using SubscriptionPtr = MuddleEndpoint::SubscriptionPtr;
  using DeadlineTimer   = moment::DeadlineTimer;
  using Rng             = random::LinearCongruentialGenerator;

  struct KademliaNode
  {
    Address       address{};
    DeadlineTimer lifetime{CLOCK_NAME};

    template <typename R, typename P>
    KademliaNode(Address address, std::chrono::duration<R, P> const &duration)
      : address{std::move(address)}
    {
      lifetime.Restart(duration);
    }
  };

  using NodeList = std::list<KademliaNode>;

  void   Periodically() override;
  void   ResolveAddresses(Addresses const &addresses);
  void   OnResolvedAddress(Address const &address, service::Promise const &promise);
  UriSet GenerateUriSet(Addresses const &addresses);
  void   OnAnnouncement(Address const &from, byte_array::ConstByteArray const &payload);
  void   ScheduleNextAnnouncement();
  void   MakeAnnouncement();
  void   UpdateKademliaPeers();

  std::string const name_;
  char const *const logging_name_{name_.c_str()};

  core::Reactor &       reactor_;
  PeerConnectionList &  connections_;
  MuddleRegister const &register_;
  MuddleEndpoint &      endpoint_;
  Address const         address_;
  rpc::Client           rpc_client_;
  SubscriptionPtr       announcement_subscription_;

  mutable Mutex     lock_;
  Peers             external_peers_;
  Rng               rng_{};
  DeadlineTimer     announcement_interval_{CLOCK_NAME};
  PeerSelectionMode mode_{PeerSelectionMode::DEFAULT};
  Addresses         desired_addresses_;
  Addresses         kademlia_addresses_;
  PendingPromised   pending_resolutions_;
  PeersInfo         peers_info_;
  NodeList          kademlia_nodes_;
};

}  // namespace muddle
}  // namespace fetch
