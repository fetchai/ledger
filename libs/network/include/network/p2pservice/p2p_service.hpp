#ifndef NETWORK_P2PSERVICE_P2P_SERVICE_HPP
#define NETWORK_P2PSERVICE_P2P_SERVICE_HPP
#include"network/service/server.hpp"
#include"network/management/connection_register.hpp"
#include"network/p2pservice/p2p_peer_details.hpp"
#include"network/details/thread_pool.hpp"

#include"network/p2pservice/p2p_identity.hpp"
#include"network/p2pservice/p2p_identity_protocol.hpp"

#include"network/p2pservice/p2p_peer_directory.hpp"
#include"network/p2pservice/p2p_peer_directory_protocol.hpp"
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


    // Listening for new connections
    this->SetConnectionRegister(register_);
    register_.OnClientEnter([](connection_handle_type const&i) {
        std::cout << "\rNew connection " << i << std::endl << ">> ";
      });

    register_.OnClientLeave([](connection_handle_type const&i) {
        std::cout << "\rPeer left " << i << std::endl << ">> ";
      });

    // Identity
    identity_ = new P2PIdentity(register_, tm);
    my_details_ = identity_->my_details();    
    identity_protocol_ = new P2PIdentityProtocol(identity_);    
    this->Add(IDENTITY, identity_protocol_);

    {
      EntryPoint discovery_ep;
      discovery_ep.is_discovery = true;
      discovery_ep.port = port;
      std::lock_guard< mutex::Mutex > lock(my_details_->mutex);
      my_details_->details.entry_points.push_back(discovery_ep);
    }
    
    // TODO(Troels): Add pk etc. to identity - ECDSA needed

    // P2P Peer Directory
    directory_ = new P2PPeerDirectory(DIRECTORY, register_, thread_pool_ );
    directory_protocol_ = new P2PPeerDirectoryProtocol(directory_);
    this->Add(DIRECTORY, directory_protocol_);
  }

  /// Events for new peer discovery
  /// @{
  // TODO, WIP(Troels): Hooks for udpating other services
  typedef std::function< void(connection_handle_type const&, PeerDetails const &) > callback_peer_connected_type;
  typedef std::function< void(connection_handle_type const&, PeerDetails const &) > callback_peer_update_type;
  typedef std::function< void(connection_handle_type const&, PeerDetails const &) > callback_peer_leave_type;
  
  void OnPeerConnected( callback_peer_connected_type const &f) 
  {
  }

  void OnPeerUpdate(callback_peer_update_type const &f ) 
  {

  }

  void OnPeerLeave( ) 
  {

  }
  
  /// @}

  
  /// Methods to interact with peers
  /// @{
  void Start() 
  {
    directory_->Start();
  }

  void Stop() 
  {
    directory_->Stop();
  }
  
  client_register_type connection_register() {
    return register_;    
  };
  
  void Connect(byte_array::ConstByteArray const &host, uint16_t const &port) 
  {
    shared_service_client_type client = register_.CreateServiceClient<client_type >( manager_, host, port);

    std::size_t n = 0;    
    while((n < 10) && (!client->is_alive())) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      ++n;
   }

    if(n >= 10 ) {
      fetch::logger.Error("Connection never came to live in P2P module");
      // TODO: throw error?
      client.reset();
      return;            
    }

    // Getting own IP seen externally
    byte_array::ByteArray address;      
    auto p = client->Call(IDENTITY, P2PIdentityProtocol::EXCHANGE_ADDRESS, host);
    p.As(address);
    
    { // Exchanging identities including node setup
      std::lock_guard< mutex::Mutex > lock(my_details_->mutex);

      // Updating IP for P2P node
      for(auto &e: my_details_->details.entry_points) {
        if(e.is_discovery) {
          // TODO: Make mechanim for verifying address          
          e.host.insert(address);
        }
      }
        
      auto p = client->Call(IDENTITY, P2PIdentityProtocol::HELLO, my_details_->details);
      PeerDetails details = p.As< PeerDetails >();

      auto regdetails = register_.GetDetails( client->handle() );
      {
        std::lock_guard< mutex::Mutex > lock( *regdetails );
        regdetails->Update(details);
      }
    }    
    
    {
      std::lock_guard< mutex_type > lock_(peers_mutex_);
      peers_[client->handle()] = client;
    }
    
  }
  /// @}

  /// Methods to add node components
  /// @{
  void AddLane(uint32_t const &lane, byte_array::ConstByteArray const &host, uint16_t const &port) 
  {
    // TODO: connect
    
    {      
      std::lock_guard< mutex::Mutex > lock(my_details_->mutex);
      EntryPoint lane_details;
      lane_details.host.insert( host );
      lane_details.port = port;
      //     lane_details.public_key = "todo";
      lane_details.lane_id = lane;
      lane_details.is_lane = true;
      my_details_->details.entry_points.push_back( lane_details );
    }
    

  }

  void AddMainChain(byte_array::ConstByteArray const &host, uint16_t const &port)
  {

    // TODO: connect
    {      
      std::lock_guard< mutex::Mutex > lock(my_details_->mutex);
      EntryPoint lane_details;

      lane_details.host.insert( host );
      lane_details.port = port;
      //     lane_details.public_key = "todo";
      lane_details.is_mainchain = true;
      my_details_->details.entry_points.push_back( lane_details );
    }
    
  }  
  /// @}
  
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

  P2PIdentity* identity_;
  P2PIdentityProtocol* identity_protocol_;

  P2PPeerDirectory  *directory_;  
  P2PPeerDirectoryProtocol *directory_protocol_;
  
  
  NodeDetails my_details_;
  
  mutex::Mutex peers_mutex_;    
  std::unordered_map< connection_handle_type, shared_service_client_type > peers_;  
};


}
}
#endif
