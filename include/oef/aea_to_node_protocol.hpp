#ifndef AEA_TO_NODE_PROTOCOL_HPP
#define AEA_TO_NODE_PROTOCOL_HPP

#include"oef/oef.hpp"

namespace fetch
{
namespace aea_to_node_protocol
{

// Build the protocol for OEF(rpc) interface
class AEAToNodeProtocol : public fetch::service::Protocol {
public:

  AEAToNodeProtocol(std::shared_ptr<oef::NodeOEF> node) : Protocol() {

    // Expose the RPC interface to the OEF, note the HttpOEF also has a pointer to the OEF
    this->Expose(AEAToNodeProtocolFn::REGISTER_INSTANCE,        new service::CallableClassMember<oef::NodeOEF, std::string(std::string agentName, schema::Instance)>(node.get(), &oef::NodeOEF::RegisterInstance) );
    this->Expose(AEAToNodeProtocolFn::QUERY,                    new service::CallableClassMember<oef::NodeOEF, std::vector<std::string>(schema::QueryModel query)>  (node.get(), &oef::NodeOEF::Query) );
    this->Expose(AEAToNodeProtocolFn::BUY_AEA_TO_NODE,          new service::CallableClassMember<oef::NodeOEF, std::string(std::string id)>  (node.get(), &oef::NodeOEF::BuyFromAEA) );

    this->Expose(AEAToNodeProtocolFn::REGISTER_FOR_CALLBACKS,   new service::CallableClassMember<oef::NodeOEF, void(uint64_t, std::string id)>(service::Callable::CLIENT_ID_ARG, node.get(), &oef::NodeOEF::RegisterCallback) );
    this->Expose(AEAToNodeProtocolFn::DEREGISTER_FOR_CALLBACKS, new service::CallableClassMember<oef::NodeOEF, void(uint64_t, std::string id)>(service::Callable::CLIENT_ID_ARG, node.get(), &oef::NodeOEF::DeregisterCallback) ); // TODO: (`HUT`) : ask troels if this is going to get the same id number
  }
};

}
}

#endif
