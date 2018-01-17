#ifndef NODE_PROTOCOL_HPP
#define NODE_PROTOCOL_HPP

#include"node_functionality.hpp"
#include"commands.hpp"

#include"rpc/service_server.hpp"

class AEAProtocol : public NodeFunctionality, public fetch::rpc::Protocol { 
public:
  AEAProtocol() : NodeFunctionality(),  fetch::rpc::Protocol() {

    using namespace fetch::rpc;
    auto register_function =  new CallableClassMember<NodeFunctionality, bool(std::string, std::string)>(this, &NodeFunctionality::RegisterType);
      
    this->Expose(AEACommands::REGISTER, register_function);
  }
};

#endif
