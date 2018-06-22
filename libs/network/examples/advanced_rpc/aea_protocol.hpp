#ifndef AEA_PROTOCOL_HPP
#define AEA_PROTOCOL_HPP

#include"aea_functionality.hpp"
#include"commands.hpp"

#include"service/server.hpp"

class AEAProtocol : public AEAFunctionality, public fetch::service::Protocol { 
public:
  AEAProtocol(std::string const&info) : AEAFunctionality(info),  fetch::service::Protocol() {

    using namespace fetch::service;
    /*
    auto get_info =  new CallableClassMember<AEAFunctionality, std::string()>(this, &AEAFunctionality::get_info);
    auto connect =  new CallableClassMember<AEAFunctionality, void(std::string, uint16_t)>(this, &AEAFunctionality::Connect);    

    */
    AEAFunctionality* controller = (AEAFunctionality*) this;    
    this->Expose(AEACommands::GET_INFO, controller, &AEAFunctionality::get_info);
    this->Expose(AEACommands::CONNECT, controller, &AEAFunctionality::Connect); 
  }
};

#endif
