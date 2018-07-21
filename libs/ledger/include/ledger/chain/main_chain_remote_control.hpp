#ifndef LEDGER_CHAIN_MAIN_CHAIN_REMOTE_HPP
#define LEDGER_CHAIN_MAIN_CHAIN_REMOTE_HPP

#include"network/service/client.hpp"
#include"ledger/chain/main_chain_controller_protocol.hpp"
#include"ledger/chain/main_chain_service.hpp"
#include"network/p2pservice/p2p_peer_details.hpp"

#include<unordered_map>
namespace fetch
{
namespace chain
{

class MainChainRemoteControl 
{
public:
  typedef service::ServiceClient service_type;
  typedef std::shared_ptr< service_type > shared_service_type;
  using mainchain_index_type = uint32_t;

  enum {
    CONTROLLER_PROTOCOL_ID = MainChainService::CONTROLLER,
    IDENTITY_PROTOCOL_ID = MainChainService::IDENTITY
  };
  
  MainChainRemoteControl() { }
  MainChainRemoteControl(MainChainRemoteControl const &other) = default;
  MainChainRemoteControl(MainChainRemoteControl &&other) = default;
  MainChainRemoteControl& operator=(MainChainRemoteControl const &other) = default;
  MainChainRemoteControl& operator=(MainChainRemoteControl &&other) = default;
  
  ~MainChainRemoteControl() = default;  

  void SetClient(shared_service_type const &client) 
  {
    client_ = client;
  }

  
  void Connect(byte_array::ByteArray const& host, uint16_t const& port) 
  {    
    if(!client_) {
      TODO_FAIL("Not connected or bad pointer");      
    }

    auto p = client_->Call(CONTROLLER_PROTOCOL_ID,MainChainControllerProtocol::CONNECT, host, port );
    p.Wait();

  }

  void TryConnect(p2p::EntryPoint const &ep) 
  {    
    if(!client_) {
      TODO_FAIL("Not connected or bad pointer");      
    }
    
    auto p = client_->Call(CONTROLLER_PROTOCOL_ID,MainChainControllerProtocol::TRY_CONNECT, ep);
    p.Wait();

  }

  
  void Shutdown() 
  {
    if(!client_) {
      TODO_FAIL("Not connected or bad pointer");      
    }

    auto p = client_->Call(CONTROLLER_PROTOCOL_ID,MainChainControllerProtocol::SHUTDOWN);
    p.Wait();      
  }
  
  int IncomingPeers(mainchain_index_type const &mainchain) 
  {    

    if(!client_) {
      TODO_FAIL("Not connected or bad pointer");      
    }
    
    auto p = client_->Call(CONTROLLER_PROTOCOL_ID,MainChainControllerProtocol::INCOMING_PEERS);
    return p.As< int >();

    return 0;        
  }
  
  int OutgoingPeers(mainchain_index_type const &mainchain) 
  {        

    if(!client_) {
      TODO_FAIL("Not connected or bad pointer");      
    }

    auto p = client_->Call(CONTROLLER_PROTOCOL_ID,MainChainControllerProtocol::OUTGOING_PEERS);
    return p.As< int >();

    return 0;    
  }

  bool IsAlive(mainchain_index_type const &mainchain) 
  {
    return bool(client_);
  }
  
private:
  shared_service_type client_;
  
};

  
}
}

#endif
