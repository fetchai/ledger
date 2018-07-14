#ifndef NETWORK_P2PSERVICE_P2P_CONNECTIVITY_CONTROLLER_HPP
#define NETWORK_P2PSERVICE_P2P_CONNECTIVITY_CONTROLLER_HPP
#include"network/service/client.hpp"
#include"network/management/connection_register.hpp"
#include"network/service/client.hpp"
#include"network/p2pservice/p2p_peer_details.hpp"


namespace fetch
{
namespace ledger
{

class P2PConnectivityController
{
public:
  using connectivity_details_type = PeerDetails;
  using client_type = fetch::network::TCPClient;  
  using service_client_type = fetch::service::ServiceClient;
  using shared_service_client_type = std::shared_ptr< service_client_type >;
  using weak_service_client_type = std::shared_ptr< service_client_type >;    
  using client_register_type = fetch::network::ConnectionRegister< connectivity_details_type >;
  using network_manager_type = fetch::network::NetworkManager;
  using mutex_type = fetch::mutex::Mutex;
  using connection_handle_type = client_register_type::connection_handle_type;
  using protocol_handler_type = service::protocol_handler_type;  
  
  P2PConnectivityController(client_register_type reg, network_manager_type nm)
    : register_(reg), manager_(nm)
  {
  }

  /// Available through protocols
  /// @{
  PeerDetails ExchangeDetails(connectivity_details_type const &client_id,
    PeerDetails details) 
  {

   // TODO: Update details
    
    std::lock_guard< mutex::Mutex > lock( my_details_mutex_ );
    return my_details_;
  }



  /// @}

  /// Internal service methods
  /// @{
  
  /// @}
  
  
private:
  mutex::Mutex mutex_;
  PeerDetails my_details_;
  

  client_register_type register_;
  network_manager_type manager_;
};


}
}
