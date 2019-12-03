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
#include "network/uri.hpp"

#include <atomic>
#include <memory>

namespace fetch {
namespace muddle {

class PeerTrackerProtocol : public service::Protocol
{
public:
  using Peers          = std::deque<PeerInfo>;
  using ConstByteArray = byte_array::ConstByteArray;
  using NetworkUris    = std::vector<network::Uri>;
  enum
  {
    PING            = 1,
    FIND_PEERS      = 2,
    GET_MUDDLE_URIS = 3,

    // TODO(tfr): Not implemented
    REQUEST_DISCONNECT = 4
  };

  explicit PeerTrackerProtocol(KademliaTable &table, NetworkUris uris = {})
    : table_{table}
    , uris_{std::move(uris)}
  {
    Expose(PING, this, &PeerTrackerProtocol::Ping);
    Expose(FIND_PEERS, this, &PeerTrackerProtocol::FindPeers);
    Expose(GET_MUDDLE_URIS, this, &PeerTrackerProtocol::GetMuddleUris);
  }

  void UpdateExternalUris(NetworkUris const &uris)
  {
    FETCH_LOCK(uri_mutex_);
    uris_ = uris;
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

  NetworkUris GetMuddleUris()
  {
    FETCH_LOCK(uri_mutex_);
    std::cout << "Getting URIs!!" << std::endl;
    return uris_;
  }

  KademliaTable &table_;

  std::mutex  uri_mutex_;
  NetworkUris uris_;
};

}  // namespace muddle
}  // namespace fetch
