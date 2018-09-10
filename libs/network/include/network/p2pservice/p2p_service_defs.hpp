#pragma once

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
    if (service_type < other.service_type) return true;
    if (service_type > other.service_type) return false;
    if (instance_number < other.instance_number) return true;
    return false;
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
