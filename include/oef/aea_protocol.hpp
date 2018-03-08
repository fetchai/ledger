#ifndef NODE_OEF_HPP
#define NODE_OEF_HPP

#include"oef/node_oef.hpp"

// Build the protocol for OEF(rpc) interface
class RpcProtocolAEA : public fetch::service::Protocol {
public:

  RpcProtocolAEA(std::shared_ptr<NodeOEF> node) : Protocol() {

    // Expose the RPC interface to the OEF, note the HttpOEF also has a pointer to the OEF
    this->Expose(AEAProtocol::REGISTER_INSTANCE,      new CallableClassMember<NodeOEF, std::string(std::string agentName, Instance)>(node.get(), &NodeOEF::RegisterInstance) );
    this->Expose(AEAProtocol::QUERY,                  new CallableClassMember<NodeOEF, std::vector<std::string>(QueryModel query)>  (node.get(), &NodeOEF::Query) );
  }
};

#endif
