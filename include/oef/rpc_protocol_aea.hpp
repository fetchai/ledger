#ifndef RPC_PROTOCOL_AEA_HPP
#define RPC_PROTOCOL_AEA_HPP

#include"oef/node_oef.hpp"

namespace fetch
{
namespace rpc_protocol_aea
{

// Build the protocol for OEF(rpc) interface
class RpcProtocolAEA : public fetch::service::Protocol {
public:

  RpcProtocolAEA(std::shared_ptr<node_oef::NodeOEF> node) : Protocol() {

    // Expose the RPC interface to the OEF, note the HttpOEF also has a pointer to the OEF
    this->Expose(AEAProtocol::REGISTER_INSTANCE,      new service::CallableClassMember<node_oef::NodeOEF, std::string(std::string agentName, schema::Instance)>(node.get(), &node_oef::NodeOEF::RegisterInstance) );
    this->Expose(AEAProtocol::QUERY,                  new service::CallableClassMember<node_oef::NodeOEF, std::vector<std::string>(schema::QueryModel query)>  (node.get(), &node_oef::NodeOEF::Query) );
  }
};

}
}

#endif
