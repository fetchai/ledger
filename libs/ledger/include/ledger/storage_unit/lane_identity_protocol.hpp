#pragma once
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
    GET_IDENTITY,    
    GET_LANE_NUMBER,
    GET_TOTAL_LANES,    
    AUTHENTICATE_CONTROLLER    
    
  };


  
  LaneIdentityProtocol(LaneIdentity* ctrl) 
  {
    this->Expose(PING, ctrl, &LaneIdentity::Ping);
    this->ExposeWithClientArg(HELLO, ctrl, &LaneIdentity::Hello);
    this->Expose(GET_IDENTITY, ctrl, &LaneIdentity::Identity);
    this->Expose(GET_LANE_NUMBER, ctrl, &LaneIdentity::GetLaneNumber) ;
    this->Expose(GET_TOTAL_LANES, ctrl, &LaneIdentity::GetTotalLanes) ;       
    this->Expose(AUTHENTICATE_CONTROLLER, ctrl, &LaneIdentity::AuthenticateController) ;   
  }
  
};

}

}

