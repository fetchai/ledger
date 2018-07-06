#ifndef LEDGER_STORAGE_UNIT_LANE_REMOTE_CONTROL_HPP
#define LEDGER_STORAGE_UNIT_LANE_REMOTE_CONTROL_HPP
#include"network/service/client.hpp"
#include"ledger/storage_unit/lane_controller_protocol.hpp"
#include"ledger/storage_unit/lane_service.hpp"

#include<unordered_map>
namespace fetch
{
namespace ledger
{

class LaneRemoteControl 
{
public:
  typedef service::ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type > shared_client_type;
  typedef std::weak_ptr< client_type > weak_client_type;  
  using lane_index_type = uint32_t;

  enum {
    PROTOCOL_ID = LaneService::CONTROLLER   
  };
  
  LaneRemoteControl() { }
  LaneRemoteControl(LaneRemoteControl const &other) = default;
  LaneRemoteControl(LaneRemoteControl &&other) = default;
  LaneRemoteControl& operator=(LaneRemoteControl const &other) = default;
  LaneRemoteControl& operator=(LaneRemoteControl &&other) = default;
  
  ~LaneRemoteControl() = default;  

  void AddClient(lane_index_type const &lane, weak_client_type const &client) 
  {
    clients_[lane] = client;    
  }

  
  void Connect(lane_index_type const &lane, byte_array::ByteArray const& host, uint16_t const& port) 
  {
    if(clients_.find(lane) == clients_.end() ) {
      TODO_FAIL("Client not found");
    }

    
    auto ptr = clients_[lane].lock();
    if(ptr) {
      auto p = ptr->Call(PROTOCOL_ID,LaneControllerProtocol::CONNECT, host, port );
      p.Wait();
      
    }
  }

  void Shutdown(lane_index_type const &lane) 
  {
    if(clients_.find(lane) == clients_.end() ) {
      TODO_FAIL("Client not found");
    }
    
    auto ptr = clients_[lane].lock();
    if(ptr) {
      auto p = ptr->Call(PROTOCOL_ID,LaneControllerProtocol::SHUTDOWN);
      p.Wait();      
    }
  }
  
  int IncomingPeers(lane_index_type const &lane) 
  {
    if(clients_.find(lane) == clients_.end() ) {
      TODO_FAIL("Client not found");
    }
    
    auto ptr = clients_[lane].lock();
    if(ptr) {
      auto p = ptr->Call(PROTOCOL_ID,LaneControllerProtocol::INCOMING_PEERS);
      return p.As< int >();
    }

    TODO_FAIL("client connection has died");

    return 0;        
  }
  
  int OutgoingPeers(lane_index_type const &lane) 
  {
    if(clients_.find(lane) == clients_.end() ) {
      TODO_FAIL("Client not found");
    }
        
    auto ptr = clients_[lane].lock();
    if(ptr) {
      auto p = ptr->Call(PROTOCOL_ID,LaneControllerProtocol::OUTGOING_PEERS);
      return p.As< int >();
    }

    TODO_FAIL("client connection has died");

    return 0;    
  }

  bool IsAlive(lane_index_type const &lane) 
  {
    if(clients_.find(lane) == clients_.end() ) {
      TODO_FAIL("Client not found");
    }
    
    auto ptr = clients_[lane].lock();
    return bool(ptr);
  }
  
private:
  std::unordered_map< lane_index_type, weak_client_type > clients_;
  
};

  
}
}

#endif
