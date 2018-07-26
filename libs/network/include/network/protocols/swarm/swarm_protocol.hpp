#pragma once

#include "network/service/protocol.hpp"
#include "commands.hpp"
#include "network/swarm/swarm_node.hpp"
#include "network/interfaces/swarm/swarm_node_interface.hpp"

namespace fetch
{
namespace swarm
{

class SwarmProtocol : public fetch::service::Protocol
{
public:
  SwarmProtocol(SwarmNodeInterface *node) : Protocol() {
      this->Expose(protocols::Swarm::CLIENT_NEEDS_PEER,
                   node,  &SwarmNodeInterface::ClientNeedsPeer);
  }
};

}
}

