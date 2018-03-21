#ifndef NODE_TO_NODE_PROTOCOL_HPP
#define NODE_TO_NODE_PROTOCOL_HPP

#include"oef/oef.hpp"
#include"protocols/node_to_node/commands.hpp"
#include"oef/message_history.hpp" // TODO: (`HUT`) : remove this, needed for debug

namespace fetch
{
namespace protocols
{

class NodeToNodeProtocol : public fetch::service::Protocol {
public:

  NodeToNodeProtocol(std::shared_ptr<oef::NodeOEF> node) : Protocol() {

    this->Expose(NodeToNodeRPC::PING,             new service::CallableClassMember<oef::NodeOEF, std::string()>(node.get(), &oef::NodeOEF::ping));
    this->Expose(NodeToNodeRPC::GET_INSTANCE,     new service::CallableClassMember<oef::NodeOEF, schema::Instance()>(node.get(), &oef::NodeOEF::getInstance) );

    // Multi-node functionality
    this->Expose(NodeToNodeRPC::FORWARD_QUERY,    new service::CallableClassMember<oef::NodeOEF, void(std::string name, schema::Endpoint endpoint, schema::QueryModelMulti queryModelMulti)>(node.get(), &oef::NodeOEF::ForwardQuery) );
    this->Expose(NodeToNodeRPC::RETURN_QUERY,     new service::CallableClassMember<oef::NodeOEF, void(schema::QueryModelMulti, std::vector<std::string>)>(node.get(), &oef::NodeOEF::ReturnQuery));

    // debug functionality
    this->Expose(NodeToNodeRPC::DBG_ADD_ENDPOINT, new service::CallableClassMember<oef::NodeOEF, void(schema::Endpoint endpoint, schema::Instance instance, schema::Endpoints endpoints)>(node.get(), &oef::NodeOEF::AddEndpoint) );

    this->Expose(NodeToNodeRPC::DBG_ADD_AGENT,    new service::CallableClassMember<oef::NodeOEF, void(schema::Endpoint endpoint, std::string agent)>(node.get(), &oef::NodeOEF::addAgent) );
    this->Expose(NodeToNodeRPC::DBG_REMOVE_AGENT, new service::CallableClassMember<oef::NodeOEF, void(schema::Endpoint endpoint, std::string agent)>(node.get(), &oef::NodeOEF::removeAgent) );

    this->Expose(NodeToNodeRPC::DBG_LOG_EVENT,    new service::CallableClassMember<oef::NodeOEF, void(schema::Endpoint endpoint, oef::Event event)>(node.get(), &oef::NodeOEF::logEvent) );
  }
};

}
}

#endif
