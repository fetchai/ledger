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

#include "network/p2pservice/manifest.hpp"
#include "network/p2pservice/p2p_managed_local_lane_service.hpp"
#include "network/p2pservice/p2p_managed_local_service.hpp"
#include "network/p2pservice/p2p_service_defs.hpp"

namespace fetch {
namespace p2p {

/**
 * This is representation of a LOCAL service which a P2P2 instance is
 * controlling. Eg; a LANE_SERVICE to whom it is handing out LANE_N peers.
 */
class P2PManagedLocalServices
{
  using Uri               = network::Uri;
  using Manifest          = network::Manifest;
  using ServiceType       = network::ServiceType;
  using ServiceIdentifier = network::ServiceIdentifier;
  using Services          = std::map<ServiceIdentifier, std::shared_ptr<P2PManagedLocalService>>;
  using ServiceIter =
      std::map<ServiceIdentifier, std::shared_ptr<P2PManagedLocalService>>::iterator;
  static constexpr char const *LOGGING_NAME = "P2PManagedLocalServices";

public:
  P2PManagedLocalServices(LaneManagement &lane_management)
    : lane_management_(lane_management)
  {}

  void MakeFromManifest(const Manifest &manifest)
  {
    manifest.ForEach([this](const ServiceIdentifier &ident, const Uri &uri) {
      switch (ident.service_type)
      {
      case ServiceType::LANE:
      {
        std::shared_ptr<P2PManagedLocalService> foo(
            new P2PManagedLocalLaneService(uri, ident, lane_management_));
        this->services_[ident] = foo;
        break;
      }

      case ServiceType::CORE:
      case ServiceType::HTTP:
        this->services_[ident] = std::make_shared<P2PManagedLocalService>(uri, ident);
        break;

      case ServiceType::INVALID:
      default:
        break;
      }
    });

    FETCH_LOG_INFO(LOGGING_NAME, "Create services count:", this->services_.size());
  }

  void Refresh()
  {
    for (auto &service : services_)
    {
      service.second->Refresh();
    }
  }

  void DistributeManifest(Manifest const &manifest)
  {
    manifest.ForEach([this](ServiceIdentifier const &ident, const Uri &uri) {
      auto const iter = services_.find(ident);
      if (iter != services_.end())
      {
        iter->second->AddPeer(uri);
      }
    });
  }

  void EraseManifest(const Manifest &manifest)
  {
    manifest.ForEach([this](ServiceIdentifier const &ident, const Uri &uri) {
      auto const iter = services_.find(ident);
      if (iter != services_.end())
      {
        iter->second->RemovePeer(uri);
      }
    });
  }

private:
  Services        services_;
  LaneManagement &lane_management_;
};

}  // namespace p2p
}  // namespace fetch
