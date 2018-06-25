#ifndef __SWARM_PROTOCOL__
#define __SWARM_PROTOCOL__

#include "network/service/protocol.hpp"
#include "commands.hpp"
#include "network/swarm/swarm_node.hpp"

namespace fetch
{
namespace swarm
{

class SwarmProtocol : public fetch::service::Protocol
{
  //TODO(katie) Check with Nathan if I need to keep a node refernce in this class
  //TODO(katie) service calls should be an enum
public:
  SwarmProtocol(std::shared_ptr<SwarmNode> node) : Protocol() {
    this->Expose(protocols::Swarm::CLIENT_NEEDS_PEER,           node.get(),  &SwarmNode::ClientNeedsPeer);
  }
};

}
}

#endif //__SWARM_PROTOCOL__
