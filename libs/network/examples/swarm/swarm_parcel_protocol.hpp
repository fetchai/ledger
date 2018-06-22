#ifndef __SWARM_PARCEL_PROTOCOL_HPP
#define __SWARM_PARCEL_PROTOCOL_HPP

#include <iostream>
#include <string>
using std::cout;
using std::cerr;
using std::string;

#include "../../include/network/service/protocol.hpp"
#include "swarm_parcel_node.hpp"

namespace fetch
{
namespace swarm
{

class SwarmParcelProtocol : public fetch::service::Protocol
{
  //TODO(katie) Check with Nathan if I need to keep a node refernce in this class
  //TODO(katie) service calls should be an enum
public:
  SwarmParcelProtocol(std::shared_ptr<SwarmParcelNode> parcelNode) : Protocol() {
    this->Expose(1,           parcelNode.get(),  &SwarmParcelNode::ClientNeedParcelList);
    this->Expose(2,           parcelNode.get(),  &SwarmParcelNode::ClientNeedParcelData);
   }
};

}
}

#endif //__SWARM_PARCEL_PROTOCOL_HPP
