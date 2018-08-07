#pragma once

namespace fetch {
namespace chain {

class MainChainIdentityProtocol : public service::Protocol
{
public:
  enum
  {
    PING = 1,
    HELLO,
    AUTHENTICATE_CONTROLLER,
    EXCHANGE_DETAILS
  };

  MainChainIdentityProtocol(MainChainIdentity *ctrl)
  {
    this->Expose(PING, ctrl, &MainChainIdentity::Ping);
    this->ExposeWithClientArg(HELLO, ctrl, &MainChainIdentity::Hello);
    this->ExposeWithClientArg(AUTHENTICATE_CONTROLLER, ctrl, &MainChainIdentity::AuthenticateController);
    this->ExposeWithClientArg(EXCHANGE_DETAILS, ctrl, &MainChainIdentity::ExchangeDetails);
  }
};

}  // namespace chain

}  // namespace fetch
