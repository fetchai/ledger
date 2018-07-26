#pragma once

namespace fetch
{
namespace chain
{


class MainChainIdentityProtocol : public service::Protocol
{
public:

  enum {
    PING = 1,
    HELLO,
    AUTHENTICATE_CONTROLLER    
    
  };
  
  MainChainIdentityProtocol(MainChainIdentity* ctrl) 
  {
    this->Expose(PING, ctrl, &MainChainIdentity::Ping);
    this->Expose(HELLO, ctrl, &MainChainIdentity::Hello);
    this->Expose(AUTHENTICATE_CONTROLLER, ctrl, &MainChainIdentity::AuthenticateController) ;   
  }
  
};

}

}

