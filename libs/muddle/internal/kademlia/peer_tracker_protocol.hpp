#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/byte_array/const_byte_array.hpp"
#include "kademlia/table.hpp"
#include "moment/clock_interfaces.hpp"
#include "muddle/address.hpp"
#include "network/service/protocol.hpp"
#include "network/uri.hpp"

#include <atomic>
#include <memory>

namespace fetch {
namespace muddle {

class PeerTrackerProtocol : public service::Protocol
{
public:
  using ClockInterface = moment::ClockInterface;
  using Clock          = ClockInterface::AccurateSystemClock;
  using Timepoint      = ClockInterface::Timestamp;
  using Duration       = ClockInterface::Duration;

  using Ports          = std::vector<uint16_t>;
  using Peers          = std::deque<PeerInfo>;
  using ConstByteArray = byte_array::ConstByteArray;
  using NetworkUris    = std::vector<network::Uri>;
  enum
  {
    PING             = 1,
    FIND_PEERS       = 2,
    GET_MUDDLE_URIS  = 3,
    GET_MUDDLE_PORTS = 4,

    // TODO(tfr): Not implemented
    REQUEST_DISCONNECT = 5
  };

  explicit PeerTrackerProtocol(KademliaTable &table, NetworkUris uris = {})
    : table_{table}
    , uris_{std::move(uris)}
  {
    Expose(PING, this, &PeerTrackerProtocol::Ping);
    Expose(FIND_PEERS, this, &PeerTrackerProtocol::FindPeers);
    Expose(GET_MUDDLE_URIS, this, &PeerTrackerProtocol::GetMuddleUris);
    Expose(GET_MUDDLE_PORTS, this, &PeerTrackerProtocol::GetMuddlePorts);
  }

  void UpdateExternalUris(NetworkUris const &uris)
  {
    FETCH_LOCK(uri_mutex_);
    uris_ = uris;
  }

  void UpdateExternalPorts(Ports const &ports)
  {
    FETCH_LOCK(uri_mutex_);
    ports_ = ports;
  }

private:
  ConstByteArray Ping()
  {
    // Notify about activity
    return "pong";
  }

  Peers FindPeers(Address const &address)
  {
    auto  peers = table_.FindPeer(address);
    Peers ret;
    for (auto &p : peers)
    {
      if (p.uri.IsValid())
      {
        ret.push_back(p);
      }
    }
    return ret;
  }

  NetworkUris GetMuddleUris()
  {
    FETCH_LOCK(uri_mutex_);
    return uris_;
  }

  Ports GetMuddlePorts()
  {
    FETCH_LOCK(uri_mutex_);
    return ports_;
  }

  KademliaTable &table_;

  Mutex       uri_mutex_;
  NetworkUris uris_;
  Ports       ports_;
};

}  // namespace muddle
}  // namespace fetch
