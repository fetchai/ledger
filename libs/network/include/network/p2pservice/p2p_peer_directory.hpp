#ifndef NETWORK_P2PSERVICE_P2P_PEER_DIRECTORY_HPP
#define NETWORK_P2PSERVICE_P2P_PEER_DIRECTORY_HPP
#include"network/service/client.hpp"
#include"network/management/connection_register.hpp"
#include"network/service/client.hpp"
#include"network/p2pservice/p2p_peer_details.hpp"
#include "network/service/publication_feed.hpp"
#include "network/service/function.hpp"
#include"core/mutex.hpp"
#include"crypto/fnv.hpp"
namespace fetch
{
namespace p2p
{

class P2PPeerDirectory : public fetch::service::HasPublicationFeed 
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
  using suggestion_map_type = std::unordered_map< byte_array::ConstByteArray, connectivity_details_type, crypto::CallableFNV  >;
  using thread_pool_type = network::ThreadPool;
  
  enum {
    FEED_ENOUGH_CONNECTIONS = 1,
    FEED_REQUEST_CONNECTIONS,
    FEED_ANNOUNCE_PEER
  };
  
  P2PPeerDirectory(uint64_t const &protocol, client_register_type reg, thread_pool_type pool)
    : protocol_(protocol), register_(reg), thread_pool_(pool) 
  {
    last_handle_ = 0;
  }

  

  
  /// RPC calls
  /// @{
  void NeedConnections(connection_handle_type const &client_id) 
  {
    auto details = register_.GetDetails(client_id);
    std::lock_guard< mutex::Mutex > lock( *details );

    AddPeer(client_id, *details);    
  }

  void EnoughConnections(connection_handle_type const &client_id) 
  {
    auto details = register_.GetDetails(client_id);    
    std::lock_guard< mutex::Mutex > lock( *details );

    RemovePeer(client_id, details->public_key);
  }
    
  suggestion_map_type SuggestPeersToConnectTo() 
  {
    std::lock_guard< mutex::Mutex > lock( suggest_mutex_ );
    return suggested_peers_;
  }
  /// @}


  /// Maintainance logic
  /// Methods to ensure that we get info from new peers
  /// @{
  void Start() 
  {
    if(running_) return;    
    running_ = true;
    thread_pool_->Post([this]() { this->ListenToNewPeers(); } );    
    
  }

  void Stop() 
  {
    if(!running_) return;
    
  }
  
  void ListenTo(std::shared_ptr< service::ServiceClient > const &client) 
  {
//    if(!running_) return;
    std::cout << "Adding hooks" << std::endl;

    // TODO: Refactor subscribe such that there is no memory leak
    connection_handle_type handle = client->handle();
    
    client->Subscribe(protocol_, FEED_REQUEST_CONNECTIONS,
      new service::Function<void(PeerDetails)>(
        [this, handle](PeerDetails const &details) {
          this->AddPeer(handle, details);
        }));
    
    client->Subscribe(protocol_, FEED_ENOUGH_CONNECTIONS,
      new service::Function<void(byte_array::ConstByteArray)>(
        [this, handle](byte_array::ConstByteArray const &public_key) {
//          auto details = register_.GetDetails(handle);
//          std::lock_guard< mutex::Mutex > lock( *details );          
          this->RemovePeer(handle, public_key);
        }));
    
    /*
    // TODO: Work out whether we want this
    ptr->Subscribe(protocol_, FEED_ANNOUNCE_PEER,
    new service::Function<void(PeerDetails)>(
    [this](PeerDetails const &details) {
    this->AnnouncePeer(details);
    }));
    */
    
  }
  

  
  void ListenToNewPeers() 
  {
    if(!running_) return;


  }

  void UpdateAllKnownPeers() 
  {
    // Updating all known peers
    thread_pool_->Post([this]() { this->ListenToNewPeers(); }, 1000 ); // TODO: Make configurable
  }
  
  /// @}
  
private:

  /// Internals for updating the register
  /// @{
  bool AddPeer(connection_handle_type const &client_id, connectivity_details_type const &details) 
  {
    std::lock_guard< mutex::Mutex > lock( suggest_mutex_ );    
    auto it = suggested_peers_.find(details.public_key);

    if(it == suggested_peers_.end()) {    
      suggested_peers_[details.public_key] = details;

      // TODO: Post to manager
      this->Publish(FEED_REQUEST_CONNECTIONS, details);
      return true;
    }
    return false;
  }
  
  bool RemovePeer(connection_handle_type const &client_id,
    byte_array::ConstByteArray const& public_key) 
  {
    std::lock_guard< mutex::Mutex > lock( suggest_mutex_ );
    auto it = suggested_peers_.find(public_key);

    if(it != suggested_peers_.end()) {
      suggested_peers_.erase(it);

      // TODO: Post to manager
      this->Publish(FEED_ENOUGH_CONNECTIONS, public_key);
      return true;
    }

    return false;
  }
  /// @}
  
  
  
  std::atomic< uint64_t > protocol_;
  
  std::atomic< bool > running_;  
  suggestion_map_type suggested_peers_;
  mutable mutex::Mutex suggest_mutex_;
  
  std::atomic< connection_handle_type > last_handle_;
  client_register_type register_;
  thread_pool_type thread_pool_;
  
};


}
}

#endif
