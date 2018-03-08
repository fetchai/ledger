#ifndef NODE_TO_AEA_PROTOCOL_HPP
#define NODE_TO_AEA_PROTOCOL_HPP

#include"oef/oef.hpp"

namespace fetch
{
namespace node_to_aea_protocol
{

// Build the protocol for OEF(rpc) interface
class NodeToAEAProtocol : public fetch::service::Protocol {
public:

  NodeToAEAProtocol(std::shared_ptr<oef::NodeOEF> node) : Protocol() {

    // Expose the RPC interface to the OEF, note the HttpOEF also has a pointer to the OEF
    this->Expose(AEAProtocol::REGISTER_INSTANCE,      new service::CallableClassMember<oef::NodeOEF, std::string(std::string agentName, schema::Instance)>(node.get(), &oef::NodeOEF::RegisterInstance) );
    this->Expose(AEAProtocol::QUERY,                  new service::CallableClassMember<oef::NodeOEF, std::vector<std::string>(schema::QueryModel query)>  (node.get(), &oef::NodeOEF::Query) );
  }
};

}
}

#endif
