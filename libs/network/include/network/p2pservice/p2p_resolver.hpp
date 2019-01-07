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
#include "core/mutex.hpp"
#include "network/p2pservice/identity_cache.hpp"
#include "network/uri.hpp"

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
  using Address  = byte_array::ConstByteArray;
  using Mutex    = mutex::Mutex;
  using PeerList = std::vector<network::Peer>;
  using PeerMap  = std::unordered_map<Address, PeerList>;
  using Uri      = network::Uri;

  explicit Resolver(IdentityCache const &cache)
    : cache_(cache)
  {}

  void Setup(Address const &address, Uri const &uri)
  {
    address_ = address;
    uri_     = uri;
  }

  Uri Query(Address const &address)
  {
    Uri uri;

    FETCH_LOG_INFO("Resolver", "Lookup address: ", byte_array::ToBase64(address));

    if (address == address_)
    {
      uri = uri_;
    }
    else
    {
      cache_.Lookup(address, uri);
    }

    return uri;
  }

private:
  IdentityCache const &cache_;    ///< The reference to the identity cache of the P2P service
  Address              address_;  ///< The address of the current node
  Uri                  uri_;      ///< The URI of the current node
};

}  // namespace p2p
}  // namespace fetch
