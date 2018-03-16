#ifndef NODE_TO_NODE_PROTOCOL_HPP
#define NODE_TO_NODE_PROTOCOL_HPP

#include"oef/oef.hpp"
#include"protocols/node_to_node/commands.hpp"

namespace fetch
{
namespace protocols
{

class NodeToNodeProtocol : public fetch::service::Protocol {
public:

  NodeToNodeProtocol(std::shared_ptr<oef::NodeOEF> node) : Protocol() {

    this->Expose(NodeToNodeRPC::PING,         new service::CallableClassMember<oef::NodeOEF, std::string()>(node.get(), &oef::NodeOEF::ping));
    this->Expose(NodeToNodeRPC::GET_INSTANCE, new service::CallableClassMember<oef::NodeOEF, schema::Instance()>(node.get(), &oef::NodeOEF::getInstance) );
    this->Expose(NodeToNodeRPC::QUERY,        new service::CallableClassMember<oef::NodeOEF, std::vector<std::string>(schema::QueryModelMulti queryModelMulti)>(node.get(), &oef::NodeOEF::QueryMulti) );
  }
};

}
}

#endif
