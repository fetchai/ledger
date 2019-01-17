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

#include <memory>

#include "network/uri.hpp"

namespace fetch {
namespace network {

enum class ServiceType : uint16_t
{
  INVALID = 0,
  LANE    = 1,
  CORE    = 2,
  HTTP    = 3,
};

inline char const *ToString(ServiceType s)
{
  switch (s)
  {
  case ServiceType::LANE:
    return "Lane";
  case ServiceType::CORE:
    return "CORE";
  case ServiceType::HTTP:
    return "HTTP";
  case ServiceType::INVALID:
  default:
    return "Invalid";
  }
}

struct ServiceIdentifier
{
  ServiceType service_type    = ServiceType::INVALID;
  uint16_t    instance_number = 0;

  ServiceIdentifier() = default;

  explicit ServiceIdentifier(ServiceType type, uint16_t instance = 0)
    : service_type(type)
    , instance_number(instance)
  {}

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

  std::string ToString(std::string const &divider = "/") const
  {
    return std::string{network::ToString(service_type)} + divider + std::to_string(instance_number);
  }

  bool operator==(ServiceIdentifier const &other) const
  {
    return (service_type == other.service_type) && (instance_number == other.instance_number);
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
  x.service_type = static_cast<ServiceType>(foo);
  // TODO(EJF): This still needs validation
}

}  // namespace network
}  // namespace fetch

namespace std {

template <>
struct hash<fetch::network::ServiceIdentifier>
{
  std::size_t operator()(fetch::network::ServiceIdentifier const &id) const
  {
    std::size_t const combined = (static_cast<std::size_t>(id.service_type) << 16) |
                                 static_cast<std::size_t>(id.instance_number);

    return hash<std::size_t>()(combined);
  }
};

}  // namespace std