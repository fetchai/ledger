#ifndef LEDGER_STORAGE_UNIT_LANE_REMOTE_IDENTITY_PROTOCOL_HPP
#define LEDGER_STORAGE_UNIT_LANE_REMOTE_IDENTITY_PROTOCOL_HPP
#include"ledger/storage_unit/lane_identity.hpp"
namespace fetch
{
namespace ledger
{


class LaneIdentityProtocol : public service::Protocol
{
public:

  enum {
    PING = 1,
    HELLO,
    GET_LANE_NUMBER,
    AUTHENTICATE_CONTROLLER    
    
  };


  
  LaneIdentityProtocol(LaneIdentity* ctrl) 
  {
    this->Expose(PING, ctrl, &LaneIdentity::Ping);
    this->Expose(HELLO, ctrl, &LaneIdentity::Hello);
    this->Expose(GET_LANE_NUMBER, ctrl, &LaneIdentity::GetLaneNumber) ;   
    this->Expose(AUTHENTICATE_CONTROLLER, ctrl, &LaneIdentity::AuthenticateController) ;   
  }
  
};

}

}

#endif
