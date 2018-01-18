#ifndef REMOTE_PROTOCOL_HPP
#define REMOTE_PROTOCOL_HPP

#include"remote_functionality.hpp"
#include"commands.hpp"

#include"rpc/service_server.hpp"

class RemoteProtocol : public RemoteFunctionality, public fetch::rpc::Protocol { 
public:
  RemoteProtocol(std::string const&info) : RemoteFunctionality(info),  fetch::rpc::Protocol() {

    using namespace fetch::rpc;
    auto get_info =  new CallableClassMember<RemoteFunctionality, std::string()>(this, &RemoteFunctionality::get_info);
    auto connect =  new CallableClassMember<RemoteFunctionality, void(std::string, uint16_t)>(this, &RemoteFunctionality::Connect);    
      
    this->Expose(RemoteCommands::GET_INFO, get_info);
    this->Expose(RemoteCommands::CONNECT, connect); 
  }
};

#endif
