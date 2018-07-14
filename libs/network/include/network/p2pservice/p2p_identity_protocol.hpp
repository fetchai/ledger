
#ifndef NETWORK_P2PSERVICE_P2P_IDENTITY_PROTOCOL_HPP
#define NETWORK_P2PSERVICE_P2P_IDENTITY_PROTOCOL_HPP
#include"network/p2pservice/p2p_identity.hpp"

namespace fetch
{
namespace p2p
{


class P2PIdentityProtocol : public service::Protocol
{
public:

  enum {
    PING = 1,
    HELLO
//    AUTHENTICATE    
  };

 
  P2PIdentityProtocol(P2PIdentity* ctrl) 
  {
    this->Expose(PING, ctrl, &P2PIdentity::Ping);
    this->ExposeWithClientArg(HELLO, ctrl, &P2PIdentity::Hello);
  }
  
};

}

}

#endif
