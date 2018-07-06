#ifndef LEDGER_STORAGE_UNIT_LANE_CONTROLLER_HPP
#define LEDGER_STORAGE_UNIT_LANE_CONTROLLER_HPP
#include"network/service/client.hpp"
#include"network/tcp/connection_register.hpp"
#include"network/service/client.hpp"
#include"ledger/storage_unit/lane_connectivity_details.hpp"

namespace fetch
{
namespace ledger
{

class LaneController
{
public:
  using connectivity_details_type = LaneConnectivityDetails;  
  using client_type = fetch::service::ServiceClient< fetch::network::TCPClient >;
  using shared_client_type = std::shared_ptr< client_type >;
  using weak_client_type = std::shared_ptr< client_type >;    
  using client_register_type = fetch::network::ConnectionRegister< connectivity_details_type >;
  using network_manager_type = fetch::network::ThreadManager;
  using mutex_type = fetch::mutex::Mutex;
  using connection_handle_type = client_register_type::connection_handle_type;
  
  LaneController(client_register_type reg, network_manager_type &nm  )
    : register_(reg), manager_(nm)
  { }
  
  
  /// External controls
  /// @{
  void RPCConnect(byte_array::ByteArray const& host, uint16_t const& port)
  {
    std::cout << "Received connect command from remote" << std::endl;
    
    Connect(host, port);    
  }

  // TODO: Move
  uint64_t Ping() 
  {
    return 1337;
  }
  
  void Authenticate(connection_handle_type const &client) 
  {
    auto details = register_.GetDetails(client);
    {
      std::lock_guard< mutex_type > lock( *details );
      details->is_controller = true;
    }
  }
  

  void Shutdown() 
  {
    TODO_FAIL("Needs to be implemented");
  }

  int IncomingPeers() 
  {
    std::lock_guard< mutex_type > lock_(clients_mutex_);
    int incoming = 0;
    for(auto &peer: clients_) {
      auto details = register_.GetDetails(peer.first);
      {
        if(details->is_peer && (!details->is_outgoing) ) {
          ++incoming;
        }
      }
      
    }
    return incoming;
  }

  int OutgoingPeers() 
  {
    std::lock_guard< mutex_type > lock_(clients_mutex_);
    int outgoing = 0;
    for(auto &peer: clients_) {
      auto details = register_.GetDetails(peer.first);
      {
        if(details->is_peer && details->is_outgoing) {
          ++outgoing;
        }
      }
      
    }
    return outgoing;
  }  
  /// @}

  /// Internal controls
  /// @{
  shared_client_type GetClient(connection_handle_type const &n) 
  {
    std::lock_guard< mutex_type > lock_(clients_mutex_);
    return clients_[n] ;
  }

  shared_client_type Connect(byte_array::ByteArray const& host, uint16_t const& port) {
    std::cout << "Connecting " << host << " " << port << std::endl;

    shared_client_type client = std::make_shared< client_type >(host, port, manager_);
    
//    shared_client_type client = register_.CreateClient<client_type>(host, port, manager_);
    std::cout << "DONE!" << std::endl;
    
    {
      std::lock_guard< mutex_type > lock_(clients_mutex_);
      clients_[client->handle()] = client;
    }

    // Setting up details such that the rest of the lane what kind of
    // connection we are dealing with.
    auto details = register_.GetDetails(client->handle());

    details->is_outgoing = true;
    details->is_peer = true;
    
    return client;
  }
  
  /// @}
private:
  client_register_type register_;
  network_manager_type &manager_;
  
  mutex::Mutex clients_mutex_;  
  std::unordered_map< connection_handle_type, weak_client_type > clients_;
  std::vector< connection_handle_type > inactive_clients_;
  
  
};

}
}

#endif
