#ifndef LEDGER_CHAIN_MAIN_CHAIN_CONTROLLER_PROTOCOL_HPP
#define LEDGER_CHAIN_MAIN_CHAIN_CONTROLLER_PROTOCOL_HPP
#include"ledger/chain/main_chain_controller.hpp"
#include"network/p2pservice/p2p_peer_details.hpp"
namespace fetch
{
namespace chain
{

class MainChainControllerProtocol : public service::Protocol
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


  
  MainChainControllerProtocol(MainChainController* ctrl) 
  {
    
    this->Expose(CONNECT, ctrl, &MainChainController::RPCConnect);
    this->Expose(TRY_CONNECT, ctrl, &MainChainController::TryConnect);
    
    this->Expose(SHUTDOWN, ctrl, &MainChainController::Shutdown);
    this->Expose(INCOMING_PEERS, ctrl, &MainChainController::IncomingPeers);
    this->Expose(OUTGOING_PEERS, ctrl, &MainChainController::OutgoingPeers);
  }
  
};

}

}

#endif
