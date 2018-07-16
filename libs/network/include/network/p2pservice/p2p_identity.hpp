#ifndef NETWORK_P2PSERVICE_P2P_IDENTITY_HPP
#define NETWORK_P2PSERVICE_P2P_IDENTITY_HPP
#include"network/service/client.hpp"
#include"network/management/connection_register.hpp"
#include"network/service/client.hpp"
#include"network/p2pservice/p2p_peer_details.hpp"
#include"network/p2pservice/node_details.hpp"

namespace fetch
{
namespace p2p
{

class P2PIdentity
{
public:
  using connectivity_details_type = PeerDetails;  
  using client_register_type = fetch::network::ConnectionRegister< connectivity_details_type >;
  using network_manager_type = fetch::network::NetworkManager;
  using mutex_type = fetch::mutex::Mutex;
  using connection_handle_type = client_register_type::connection_handle_type;
  using ping_type = uint32_t;
  using lane_type = uint32_t;  

  enum {
    PING = 1,
    HELLO,
    UPDATE_DETAILS, 
    EXCHANGE_ADDRESS
//    AUTHENTICATE    
  };
  
  enum {
    PING_MAGIC = 1337
  };

  P2PIdentity(uint64_t const &protocol, client_register_type reg, network_manager_type nm )
    : register_(reg), manager_(nm)
  {
    protocol_ = protocol;
    my_details_ = MakeNodeDetails();
  }

  /// External RPC callable
  /// @{
  ping_type Ping() 
  {
    return PING_MAGIC;
  }

  byte_array::ConstByteArray ExchangeAddress(connection_handle_type const &cid, byte_array::ByteArray const &address) 
  {
    {
      std::lock_guard< mutex::Mutex > lock(my_details_->mutex);
      for(auto &e: my_details_->details.entry_points)
      {
        if(e.is_discovery) {
          // TODO: Make mechanim for verifying address
          e.host.insert(address);
        }
      }
    }
        
    auto client = register_.GetClient(cid);
    if(!client) {
      return "";
    }
    
    return client->Address();
  }
  
  PeerDetails Hello(connection_handle_type const &client, PeerDetails const&pd) 
  {    
    auto details = register_.GetDetails(client);
    
    {
      std::lock_guard< mutex::Mutex > lock(*details);
      details->Update(pd);
    }
    
    std::lock_guard< mutex::Mutex > l(my_details_->mutex);
    return my_details_->details;
  }

  void UpdateDetails(connection_handle_type const &client, PeerDetails const&pd) 
  {    
    auto details = register_.GetDetails(client);
    
    {
      std::lock_guard< mutex::Mutex > lock(*details);
      details->Update(pd);
    }
  }  
  /// @}


  ///
  /// @{
  void PublishProfile()
  {
    std::lock_guard< mutex::Mutex > l(my_details_->mutex);
    register_.WithServices([this](network::AbstractConnectionRegister::service_map_type const &map) {
        for(auto const &p: map) {
          auto wptr = p.second;
          auto peer = wptr.lock();
          if(peer) {
            peer->Call(protocol_, UPDATE_DETAILS, my_details_->details);
          }
        }
      });
  }
  
  /// @}
  
  void WithOwnDetails(std::function< void(PeerDetails const &) > const &f) 
  {
    std::lock_guard< mutex::Mutex > l(my_details_->mutex);
    f(my_details_->details);
  }

  NodeDetails my_details() const {
    return my_details_;
  }
  
private:
  std::atomic< uint64_t > protocol_;  
  client_register_type register_;  
  network_manager_type manager_;
  
  NodeDetails my_details_;
};

}
}

#endif

