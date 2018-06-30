#ifndef SWARM_PROTOCOL__
#define SWARM_PROTOCOL__

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
  //TODO(katie) Check with Nathan if I need to keep a node refernce in this class
  //TODO(katie) service calls should be an enum
public:
  SwarmProtocol(SwarmNodeInterface *node) : Protocol() {
    this->Expose(protocols::Swarm::CLIENT_NEEDS_PEER,           node,  &SwarmNodeInterface::ClientNeedsPeer);
  }
};

}
}

#endif //SWARM_PROTOCOL__
