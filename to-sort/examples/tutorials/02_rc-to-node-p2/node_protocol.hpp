#pragma once
#include"vector_serialize.hpp"
#include"node_functionality.hpp"
#include"commands.hpp"

#include"rpc/service_server.hpp"

class AEAProtocol : public NodeFunctionality, public fetch::rpc::Protocol { 
public:
  AEAProtocol() : NodeFunctionality(),  fetch::rpc::Protocol() {

    using namespace fetch::rpc;
    auto register_function =  new CallableClassMember<NodeFunctionality, bool(std::string, std::string)>(this, &NodeFunctionality::RegisterType);

    auto push_data =  new CallableClassMember<NodeFunctionality, int(std::string, std::vector< double>)>(this, &NodeFunctionality::PushData);
      
    this->Expose(AEACommands::REGISTER, register_function);
    this->Expose(AEACommands::PUSH_DATA, push_data);    
  }
};

