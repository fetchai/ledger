#ifndef NETWORK_P2PSERVICE_P2P_SERVICE_HPP
#define NETWORK_P2PSERVICE_P2P_SERVICE_HPP
#include"network/service/server.hpp"
#include"network/management/connection_register.hpp"
#include"network/p2pservice/p2p_peer_details.hpp"
#include"network/details/thread_pool.hpp"
namespace fetch
{
namespace p2p
{

class P2PService : public service::ServiceServer< fetch::network::TCPServer >
{
public:
  using connectivity_details_type = PeerDetails;
  using client_register_type = fetch::network::ConnectionRegister< connectivity_details_type >;  
//  using identity_type = LaneIdentity;
//  using identity_protocol_type = LaneIdentityProtocol;   
  using connection_handle_type = client_register_type::connection_handle_type;
  
  using client_type = fetch::network::TCPClient;  
  using super_type = service::ServiceServer< fetch::network::TCPServer >;
  
  using thread_pool_type = network::ThreadPool;  
  using service_client_type = fetch::service::ServiceClient;
  using shared_service_client_type = std::shared_ptr< service_client_type >;
  using network_manager_type = fetch::network::NetworkManager;  
  using mutex_type = fetch::mutex::Mutex;
  
  enum {
    IDENTITY = 1,
    DIRECTORY
  };

  P2PService(uint16_t port, fetch::network::NetworkManager tm)
    : super_type(port, tm), manager_(tm)  
  {
    thread_pool_ = network::MakeThreadPool(1);

    
  }

  void Connect(byte_array::ConstByteArray const &host, uint16_t const &port) 
  {
    shared_service_client_type client = register_.CreateServiceClient<client_type >( manager_, host, port);

    {
      std::lock_guard< mutex_type > lock_(peers_mutex_);
      peers_[client->handle()] = client;
    }
    
  }

protected:
  /// Client setup pipeline
  /// @{
  void Handshake() 
  {

  }

  void EstablishNonce() 
  {

  }  
  /// @}
  
  
  /// Service maintainance
  /// @{  
  void PrunePeers() 
  {

  }

  void ConnectToNewPeers() 
  {

  }
  
  

  /// @}
  
  
private:
  network_manager_type manager_;
  client_register_type register_;
  thread_pool_type thread_pool_;

  mutex::Mutex peers_mutex_;    
  std::unordered_map< connection_handle_type, shared_service_client_type > peers_;  
};


}
}
#endif
