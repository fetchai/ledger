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

#include "core/mutex.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/fnv.hpp"
#include "network/peer.hpp"

#include <cstdint>
#include <vector>

namespace fetch {
namespace p2p {

/**
 * P2P service that allows nodes to try and resolve public keys to known
 * IP addresses and ports
 */
class Resolver
{
public:
  using Address = byte_array::ConstByteArray;
  using Mutex = mutex::Mutex;
  using PeerList = std::vector<network::Peer>;
  using PeerMap = std::unordered_map<Address, PeerList>;

  PeerList Query(Address const &address)
  {
    {
      FETCH_LOCK(lock_);

      auto it = map_.find(address);
      if (it != map_.end())
      {
        return it->second;
      }
    }

    return {};
  }

private:

  Mutex   lock_{__LINE__, __FILE__};
  PeerMap map_;
};

} // namespace p2p
} // namespace fetch
