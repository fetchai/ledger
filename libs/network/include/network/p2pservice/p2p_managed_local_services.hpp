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

#include <network/p2pservice/p2p_managed_local_service_state_machine.hpp>
#include <network/p2pservice/p2p_managed_local_service.hpp>
#include <network/p2pservice/p2p_service_defs.hpp>

namespace fetch {
namespace p2p {


/*******
 * This is representation of a LOCAL service which a P2P2 instance is
 * controlling. Eg; a LANE_SERVICE to whom it is handing out LANE_N peers.
 */


class P2PManagedLocalServices
{
  using Uri = network::Uri;
  using Manifest = network::Manifest;
  using ServiceType = network::ServiceType;
  using ServiceIdentifier = network::ServiceIdentifier;
  using Services = std::map<ServiceIdentifier, std::shared_ptr<P2PManagedLocalService>>;
  using ServiceIter = std::map<ServiceIdentifier, std::shared_ptr<P2PManagedLocalService>>::iterator;
  static constexpr char const *LOGGING_NAME = "P2PManagedLocalServices";

public:
  P2PManagedLocalServices(const Manifest &manifest)
  {
    MakeFromManifest(manifest);
  }

  P2PManagedLocalServices()
  {
  }

  void MakeFromManifest(const Manifest &manifest)
  {
    manifest.ForEach([this](const ServiceIdentifier &ident, const Uri &uri){
        this -> services_[ ident ] = std::make_shared<P2PManagedLocalService>(uri, ident);
      });
  }

  void Refresh()
  {
    for(auto &service : services_)
    {
      service . second -> Refresh();
    }
  }

  void DistributeManifest(const Manifest &manifest)
  {
    manifest.ForEach([this](const ServiceIdentifier &ident, const Uri &uri){
        auto iter = this -> services_ . find(ident);
        if (iter != this -> services_ . end())
        {
          iter -> second -> AddPeer(uri);
        }
      });
  }
  void EraseManifest(const Manifest &manifest)
  {
    manifest.ForEach([this](const ServiceIdentifier &ident, const Uri &uri){
        auto iter = this -> services_ . find(ident);
        if (iter != this -> services_ . end())
        {
          iter -> second -> RemovePeer(uri);
        }
      });
  }
private:
  Services services_;
};


}
}
