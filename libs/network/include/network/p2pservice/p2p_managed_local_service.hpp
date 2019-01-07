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

#include "network/p2pservice/p2p_service_defs.hpp"

namespace fetch {
namespace p2p {

/**
 * This is representation of a LOCAL service which a P2P2 instance is
 * controlling. Eg; a LANE_SERVICE to whom it is handing out LANE_N peers.
 */
class P2PManagedLocalService
{
public:
  using Uri               = network::Uri;
  using ServiceType       = network::ServiceType;
  using ServiceIdentifier = network::ServiceIdentifier;
  using Peers             = std::unordered_set<Uri>;

  static constexpr char const *LOGGING_NAME = "P2PManagedLocalService";

public:
  P2PManagedLocalService(Uri uri, ServiceIdentifier service_identifier)
    : uri_(std::move(uri))
    , service_identifier_(service_identifier)
  {}

  virtual ~P2PManagedLocalService() = default;

  virtual void AddPeer(Uri remote_uri)
  {
    peers_.insert(remote_uri);
  }

  virtual void RemovePeer(Uri remote_uri)
  {
    peers_.erase(remote_uri);
  }

  virtual void Refresh()
  {}

  const std::unordered_set<Uri> &peers() const
  {
    return peers_;
  }

  uint32_t instance_number() const
  {
    return service_identifier_.instance_number;
  }

private:
  Uri               uri_;
  ServiceIdentifier service_identifier_;
  Peers             peers_;
};

}  // namespace p2p
}  // namespace fetch
