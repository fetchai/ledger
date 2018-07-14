#ifndef NETWORK_P2PSERVICE_P2P_PEER_DIRECTORY_HPP
#define NETWORK_P2PSERVICE_P2P_PEER_DIRECTORY_HPP
#include"network/service/client.hpp"
#include"network/management/connection_register.hpp"
#include"network/service/client.hpp"
#include"network/p2pservice/p2p_peer_details.hpp"
#include "service/publication_feed.hpp"

namespace fetch
{
namespace ledger
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
  using suggestion_map_type = std::map< byte_array::ConstByteArray, connectivity_details_type >;
  using thread_pool_type = network::ThreadPool;
  
  enum {
    FEED_ENOUGH_CONNECTIONS = 1,
    FEED_REQUEST_CONNECTIONS,
    FEED_ANNOUNCE_PEER
  };
  
  P2PPeerDirectory(client_register_type reg, thread_pool_type pool)
    : register_(reg), thread_pool_(pool) 
  {
    last_handle_ = 0;
  }


  /// RPC calls
  /// @{
  void NeedConnections(connectivity_details_type const &client_id) 
  {
    register_.GetDetails(client_id);
    std::lock_guard< fetch::Mutex > lock( details );

    AddPeer(client_id, *details);    
  }

  void EnoughConnetions(connectivity_details_type const &client_id) 
  {
    register_.GetDetails(client_id);    
    std::lock_guard< fetch::Mutex > lock( details );

    RemovePeer(client_id, details->public_key);
  }
    
  std::map< PeerDetails > SuggestPeersToConnectTo() 
  {
    std::lock_guard< fetch::Mutex > lock( suggest_mutex_ );
    return suggested_peers_;
  }
  /// @}


  /// Internals for updating the register
  /// @{
  bool AddPeer(connectivity_details_type const &client_id, PeerDetails const &details) 
  {
    std::lock_guard< fetch::Mutex > lock( suggest_mutex_ );    
    auto it = suggested_peers_.find(public_key);

    if(it == suggested_peers_.end()) {    
      suggested_peers_[details.public_key] = details;

      // TODO: Post to manager
      this->Publish(P2PPeerDirector::FEED_REQUEST_CONNECTIONS, details);
      return true;
    }
    return false;
  }
  
  void RemovePeer(connectivity_details_type const &client_id, byte_array::ConstByteArray const& public_key) 
  {
    std::lock_guard< fetch::Mutex > lock( suggest_mutex_ );
    auto it = suggested_peers_.find(public_key);

    if(it != suggested_peers_.end()) {
      suggested_peers_.erase(it);

      // TODO: Post to manager
      this->Publish(P2PPeerDirector::FEED_REQUEST_CONNECTIONS, public_key);
      return true;
    }

    return false;
  }
  /// @}
  

  /// Maintainance logic
  /// Methods to ensure that we get info from new peers
  /// @{
  void Start() 
  {
    fetch::logger.Debug("Starting syncronisation of ", typeid(T).name());    
    if(running_) return;    
    running_ = true;
    thread_pool_->Post([this]() { this->ListenToNewPeers(); } );    
    
  }

  void Stop() 
  {
    if(!running_) return;
    
  }

  void ListenToNewPeers() 
  {
    if(!running_) return;
    
    register_.WithServices([this](service_map_type const &map) {
        for(auto const &p: map) {
          if(!running_) return;

            auto peer = p.second;
            auto ptr = peer.lock();
            auto details = register_.GetDetails(p.first);
            // TODO: Check if we got suggestions and add them
          
          if(p.first > last_handle_) {
            last_handle_ = p.first;
            

            // TODO: Refactor subscribe such that there is no memory leak
            connection_handle_type client = p.first;            
            ptr->Subscribe(protocol_, FEED_REQUEST_CONNECTIONS,
              new service::Function<void(PeerDetails)>(
                [this, client](PeerDetails const &details) {
                  this->AddPeer(client, details);
                }));

            ptr->Subscribe(protocol_, FEED_ENOUGH_CONNECTIONS,
              new service::Function<void(PeerDetails)>(
                [this, client](PeerDetails const &details) {
                  this->RemovePeer(client, details);                  
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
          
          
        }
      });

    thread_pool_->Post([this]() { this->ListenToNewPeers(); }, 1000 ); // TODO: Make configurable

  }
  /// @}
  
private:
  std::atomic< bool > running_;  
  suggestion_map_type suggested_peers_;
  std::atomic< connection_handle_type > last_handle_;
  thread_pool_type thread_pool_;
  
};


}
}
