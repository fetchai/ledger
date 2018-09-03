#pragma once

#include <memory>

namespace fetch {

namespace network {

enum ServiceType {
  MAINCHAIN = 0,
  LANE = 1,
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

using Uri = std::string;

}
}
