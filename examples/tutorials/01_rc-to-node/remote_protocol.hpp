#ifndef REMOTE_PROTOCOL_HPP
#define REMOTE_PROTOCOL_HPP

#include"remote_functionality.hpp"
#include"commands.hpp"

#include"service/server.hpp"

class RemoteProtocol : public RemoteFunctionality, public fetch::service::Protocol { 
public:
  RemoteProtocol(std::string const&info) : RemoteFunctionality(info),  fetch::service::Protocol() {

    using namespace fetch::service;
    auto get_info =  new CallableClassMember<RemoteFunctionality, std::string()>(this, &RemoteFunctionality::get_info);
    auto connect =  new CallableClassMember<RemoteFunctionality, void(std::string, uint16_t)>(this, &RemoteFunctionality::Connect);    
      
    this->Expose(RemoteCommands::GET_INFO, get_info);
    this->Expose(RemoteCommands::CONNECT, connect); 
  }
};

#endif
