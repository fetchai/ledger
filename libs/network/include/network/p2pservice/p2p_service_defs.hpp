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

#include <memory>

#include "network/uri.hpp"

namespace fetch {

namespace network {

enum ServiceType : uint16_t {
  MAINCHAIN = 0,
  LANE = 1,
  P2P = 2,
  HTTP = 3,
};

struct ServiceIdentifier
{
  ServiceType service_type;
  uint32_t instance_number;

  bool operator<(const ServiceIdentifier &other) const
  {
    if (service_type < other.service_type)
    {
      return true;
    }
    if (service_type > other.service_type)
    {
      return false;
    }
    if (instance_number < other.instance_number)
    {
      return true;
    }
    return false;
  }

  std::string ToString() const
  {
    const char *names[] = {
      "MAINCHAIN",
      "LANE",
      "P2P",
      "HTTP",
    };
    const char *p = names[service_type];
    return std::string(p) + "/" + std::to_string(instance_number);
  }
};

template <typename T>
void Serialize(T &serializer, ServiceIdentifier const &x)
{
  serializer << static_cast<uint16_t>(x.service_type) << x.instance_number;
}

template <typename T>
void Deserialize(T &serializer, ServiceIdentifier &x)
{
  uint16_t foo;
  serializer >> foo >> x.instance_number;
  x.service_type  = static_cast<ServiceType>(foo);
  // TODO(EJF): This still needs validation
}

}
}
