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

#include "core/byte_array/const_byte_array.hpp"
#include "kademlia/table.hpp"
#include "muddle/address.hpp"
#include "network/service/protocol.hpp"

#include <atomic>
#include <memory>

namespace fetch {
namespace muddle {

class PeerTrackerProtocol : public service::Protocol
{
public:
  using Peers          = std::deque<PeerInfo>;
  using ConstByteArray = byte_array::ConstByteArray;
  using PortsList      = std::vector<uint16_t>;

  enum
  {
    PING             = 1,
    FIND_PEERS       = 2,
    GET_MUDDLE_PORTS = 3,

    // TODO: Not implemented
    REQUEST_DISCONNECT = 4
  };

  PeerTrackerProtocol(KademliaTable &table, PortsList ports = {})
    : table_{table}
    , ports_{std::move(ports)}
  {
    Expose(PING, this, &PeerTrackerProtocol::Ping);
    Expose(FIND_PEERS, this, &PeerTrackerProtocol::FindPeers);
    Expose(GET_MUDDLE_PORTS, this, &PeerTrackerProtocol::GetMuddlePort);
  }

  void SetMuddlePorts(PortsList const &ports)
  {
    FETCH_LOCK(ports_mutex_);
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
    return table_.FindPeer(address);
  }

  PortsList GetMuddlePort()
  {
    FETCH_LOCK(ports_mutex_);
    return ports_;
  }

  KademliaTable &table_;

  std::mutex            ports_mutex_;
  std::vector<uint16_t> ports_;
};

}  // namespace muddle
}  // namespace fetch
