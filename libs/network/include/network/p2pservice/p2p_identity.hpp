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

#include "network/management/connection_register.hpp"
#include "network/p2pservice/node_details.hpp"
#include "network/p2pservice/p2p_peer_details.hpp"
#include "network/service/service_client.hpp"

namespace fetch {
namespace p2p {

class P2PIdentity
{
public:
  using connectivity_details_type = PeerDetails;
  using client_register_type      = fetch::network::ConnectionRegister<connectivity_details_type>;
  using network_manager_type      = fetch::network::NetworkManager;
  using mutex_type                = fetch::mutex::Mutex;
  using connection_handle_type    = client_register_type::connection_handle_type;
  using ping_type                 = uint32_t;
  using lane_type                 = uint32_t;

  enum
  {
    PING = 1,
    HELLO,
    UPDATE_DETAILS,
    EXCHANGE_ADDRESS
    //    AUTHENTICATE
  };

  enum
  {
    PING_MAGIC = 1337
  };

  P2PIdentity(uint64_t const &protocol, client_register_type reg, network_manager_type const &nm)
    : register_(std::move(reg)), manager_(nm)
  {
    protocol_       = protocol;
    my_details_     = MakeNodeDetails();
    profile_update_ = false;
  }

  /// External RPC callable
  /// @{
  ping_type Ping()
  {
    return PING_MAGIC;
  }

  byte_array::ConstByteArray ExchangeAddress(connection_handle_type const &cid,
                                             byte_array::ByteArray const & address)
  {
    {
      // TODO(issue 24): try not to lock mutexes that belong to other classes
      std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
      for (auto &e : my_details_->details.entry_points)
      {
        if (e.is_discovery)
        {
          // TODO(issue 24): Make mechanim for verifying address
          e.host.insert(address);
        }
      }
    }

    auto client = register_.GetClient(cid);
    if (!client)
    {
      return "";
    }

    return client->Address();
  }

  PeerDetails Hello(connection_handle_type const &client, PeerDetails const &pd)
  {
    auto details = register_.GetDetails(client);

    {
      std::lock_guard<mutex::Mutex> lock(*details);
      details->Update(pd);
    }

    std::lock_guard<mutex::Mutex> l(my_details_->mutex);
    return my_details_->details;
  }

  void UpdateDetails(connection_handle_type const &client, PeerDetails const &pd)
  {
    auto details = register_.GetDetails(client);

    {
      std::lock_guard<mutex::Mutex> lock(*details);
      details->Update(pd);
    }
  }
  /// @}

  /// Profile maintainance
  /// @{
  void PublishProfile()
  {
    std::lock_guard<mutex::Mutex> l(my_details_->mutex);
    register_.WithServices(
        [this](network::AbstractConnectionRegister::service_map_type const &map) {
          for (auto const &p : map)
          {
            auto wptr = p.second;
            auto peer = wptr.lock();
            if (peer)
            {
              peer->Call(protocol_, UPDATE_DETAILS, my_details_->details);
            }
          }
        });
    profile_update_ = false;
  }

  void MarkProfileAsUpdated()
  {
    profile_update_ = true;
  }
  /// @}

  void WithOwnDetails(std::function<void(PeerDetails const &)> const &f)
  {
    std::lock_guard<mutex::Mutex> l(my_details_->mutex);
    f(my_details_->details);
  }

  NodeDetails my_details() const
  {
    return my_details_;
  }

private:
  std::atomic<uint64_t> protocol_;
  client_register_type  register_;
  network_manager_type  manager_;
  std::atomic<bool>     profile_update_;

  NodeDetails my_details_;
};

}  // namespace p2p
}  // namespace fetch
