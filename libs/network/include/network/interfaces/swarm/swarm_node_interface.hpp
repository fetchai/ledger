#pragma once

#include <iostream>
#include <string>

#include "network/protocols/fetch_protocols.hpp"

namespace fetch {
namespace swarm {

class SwarmProtocol;

class SwarmNodeInterface
{
public:
  static const uint32_t protocol_number = fetch::protocols::FetchProtocols::SWARM;
  typedef SwarmProtocol protocol_class_type;

  SwarmNodeInterface() {}

  virtual ~SwarmNodeInterface() {}

  virtual std::string ClientNeedsPeer() = 0;
};

}  // namespace swarm
}  // namespace fetch
