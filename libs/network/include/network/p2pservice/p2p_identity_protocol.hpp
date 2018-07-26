
#pragma once
#include "network/p2pservice/p2p_identity.hpp"

namespace fetch {
namespace p2p {

class P2PIdentityProtocol : public service::Protocol
{
public:
  enum
  {
    PING             = P2PIdentity::PING,
    HELLO            = P2PIdentity::HELLO,
    UPDATE_DETAILS   = P2PIdentity::UPDATE_DETAILS,
    EXCHANGE_ADDRESS = P2PIdentity::EXCHANGE_ADDRESS
    //    AUTHENTICATE
  };

  P2PIdentityProtocol(P2PIdentity *ctrl)
  {
    this->Expose(PING, ctrl, &P2PIdentity::Ping);
    this->ExposeWithClientArg(HELLO, ctrl, &P2PIdentity::Hello);
    this->ExposeWithClientArg(UPDATE_DETAILS, ctrl,
                              &P2PIdentity::UpdateDetails);
    this->ExposeWithClientArg(EXCHANGE_ADDRESS, ctrl,
                              &P2PIdentity::ExchangeAddress);
  }
};

}  // namespace p2p

}  // namespace fetch

