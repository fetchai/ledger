#ifndef LEDGER_CHAIN_MAIN_CHAIN_IDENTITY_PROTOCOL_HPP
#define LEDGER_CHAIN_MAIN_CHAIN_IDENTITY_PROTOCOL_HPP

namespace fetch {
namespace chain {

class MainChainIdentityProtocol : public service::Protocol
{
public:
  enum
  {
    PING = 1,
    HELLO,
    AUTHENTICATE_CONTROLLER

  };

  MainChainIdentityProtocol(MainChainIdentity *ctrl)
  {
    this->Expose(PING, ctrl, &MainChainIdentity::Ping);
    this->Expose(HELLO, ctrl, &MainChainIdentity::Hello);
    this->Expose(AUTHENTICATE_CONTROLLER, ctrl,
                 &MainChainIdentity::AuthenticateController);
  }
};

}  // namespace chain

}  // namespace fetch

#endif
