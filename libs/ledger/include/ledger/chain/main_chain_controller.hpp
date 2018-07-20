#ifndef LEDGER_CHAIN_MAIN_CHAIN_CONTROLLER_HPP
#define LEDGER_CHAIN_MAIN_CHAIN_CONTROLLER_HPP
#include"network/service/client.hpp"
#include"network/management/connection_register.hpp"
#include"network/service/client.hpp"
#include"ledger/chain/main_chain_identity.hpp"
#include"ledger/chain/main_chain_identity_protocol.hpp"
#include"ledger/chain/main_chain_details.hpp"

namespace fetch
{
namespace chain
{

class MainChainController
{
public:
  using connectivity_details_type = MainChainDetails;
  using client_type = fetch::network::TCPClient;  
  using service_client_type = fetch::service::ServiceClient;
  using shared_service_client_type = std::shared_ptr< service_client_type >;
  using weak_service_client_type = std::shared_ptr< service_client_type >;    
  using client_register_type = fetch::network::ConnectionRegister< connectivity_details_type >;
  using network_manager_type = fetch::network::NetworkManager;
  using mutex_type = fetch::mutex::Mutex;
  using connection_handle_type = client_register_type::connection_handle_type;
  using protocol_handler_type = service::protocol_handler_type;  
  
  MainChainController(protocol_handler_type const &identity_protocol,
    std::weak_ptr< MainChainIdentity > const&identity,
    client_register_type reg, network_manager_type nm  )
    : identity_protocol_(identity_protocol), identity_(identity),
      register_(reg), manager_(nm)
  {

  }
  
  
  /// External controls
  /// @{
  void RPCConnect(byte_array::ByteArray const& host, uint16_t const& port)
  {
    Connect(host, port);
  }

  
  void Shutdown() 
  {
    TODO_FAIL("Needs to be implemented");
  }

  void StartSync() 
  {
    TODO_FAIL("Needs to be implemented");
  }

  void StopSync() 
  {
    TODO_FAIL("Needs to be implemented");
  }
  
  int IncomingPeers() 
  {
    std::lock_guard< mutex_type > lock_(services_mutex_);
    int incoming = 0;
    for(auto &peer: services_) {
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
    std::lock_guard< mutex_type > lock_(services_mutex_);
    int outgoing = 0;
    for(auto &peer: services_) {
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
  shared_service_client_type GetClient(connection_handle_type const &n) 
  {
    std::lock_guard< mutex_type > lock_(services_mutex_);
    return services_[n] ;
  }

  
  shared_service_client_type Connect(byte_array::ByteArray const& host, uint16_t const& port) {
    shared_service_client_type client = register_.CreateServiceClient<client_type >( manager_, host, port);

    auto ident = identity_.lock();
    if(!ident) {
      // TODO : Throw exception      
      TODO_FAIL("Identity lost");
    }
    

    // Waiting for connection to be open
    std::size_t n = 0;    
    while( n < 10 ){
      auto p = client->Call(identity_protocol_, MainChainIdentityProtocol::PING);
      if(p.Wait(100, false)) {
        if(p.As<MainChainIdentity::ping_type >() != MainChainIdentity::PING_MAGIC) {
          n = 10;          
        }
        break;        
      }
      ++n;
    }
    
    if(n >= 10) {
      logger.Warn("Connection timed out - closing");
      client->Close();
      client.reset();
      return nullptr;      
    }
          
    // Exchaning info             
    {
      std::lock_guard< mutex_type > lock_(services_mutex_);
      services_[client->handle()] = client;
    }

    // Setting up details such that the rest of the mainchain what kind of
    // connection we are dealing with.
    auto details = register_.GetDetails(client->handle());
    
    details->is_outgoing = true;
    details->is_peer = true;
    
    return client;
    
  }
  
  /// @}
private:
  protocol_handler_type identity_protocol_;
  std::weak_ptr< MainChainIdentity > identity_;  
  client_register_type register_;
  network_manager_type manager_;

  mutex::Mutex services_mutex_;  
  std::unordered_map< connection_handle_type, shared_service_client_type > services_;
  std::vector< connection_handle_type > inactive_services_;
  
  
};

}
}

#endif
