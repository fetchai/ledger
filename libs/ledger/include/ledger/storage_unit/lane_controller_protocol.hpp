#ifndef LEDGER_STORAGE_UNIT_LANE_REMOTE_CONTROL_PROTOCOL_HPP
#define LEDGER_STORAGE_UNIT_LANE_REMOTE_CONTROL_PROTOCOL_HPP
#include"ledger/storage_unit/lane_controller.hpp"
namespace fetch
{
namespace ledger
{

class LaneControllerProtocol : public service::Protocol
{
public:

  enum {
    CONNECT = 1,
    TRY_CONNECT,
    SHUTDOWN,
    START_SYNC,
    STOP_SYNC,
    INCOMING_PEERS,
    OUTGOING_PEERS
  };


  
  LaneControllerProtocol(LaneController* ctrl) 
  {
    
    this->Expose(CONNECT, ctrl, &LaneController::RPCConnect);
    this->Expose(TRY_CONNECT, ctrl, &LaneController::TryConnect);    
    this->Expose(SHUTDOWN, ctrl, &LaneController::Shutdown);
    this->Expose(INCOMING_PEERS, ctrl, &LaneController::IncomingPeers);
    this->Expose(OUTGOING_PEERS, ctrl, &LaneController::OutgoingPeers);
  }
  
};

}

}

#endif
