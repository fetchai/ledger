#ifndef __SWARM_PARCEL_PROTOCOL_HPP
#define __SWARM_PARCEL_PROTOCOL_HPP

#include <iostream>
#include <string>
using std::cout;
using std::cerr;
using std::string;

#include "network/service/protocol.hpp"
#include "commands.hpp"
#include "network/interfaces/parcels/swarm_parcel_node_interface.hpp"

namespace fetch
{
namespace swarm
{

class SwarmParcelProtocol : public fetch::service::Protocol
{
  //TODO(katie) Check with Nathan if I need to keep a node refernce in this class
  //TODO(katie) service calls should be an enum
public:
  SwarmParcelProtocol(SwarmParcelNodeInterface *node) : Protocol() {
    this->Expose(protocols::SwarmParcels::CLIENT_NEEDS_PARCEL_IDS,           node,  &SwarmParcelNodeInterface::ClientNeedParcelList);
    this->Expose(protocols::SwarmParcels::CLIENT_NEEDS_PARCEL_DATA,          node,  &SwarmParcelNodeInterface::ClientNeedParcelData);
   }
};

}
}

#endif //__SWARM_PARCEL_PROTOCOL_HPP
