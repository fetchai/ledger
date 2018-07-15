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
    PING_MAGIC = 1337
  };

  P2PIdentity(client_register_type reg, network_manager_type nm )
    : register_(reg), manager_(nm)
  {
    my_details_ = MakeNodeDetails();
  }

  /// External controls
  /// @{
  ping_type Ping() 
  {
    return PING_MAGIC;
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
  /// @}

  void PrintMyDetails() 
  {
    std::lock_guard< mutex::Mutex > l(my_details_->mutex);
    std::cout << "My details: " << std::endl;
    for(auto &e: my_details_->details.entry_points) {
      std::cout << " - " << e.host << " " << e.port << std::endl;
      }    
  }
  
  
  void WithOwnDetails(std::function< void(PeerDetails const &) > const &f) 
  {
    std::lock_guard< mutex::Mutex > l(my_details_->mutex);
    f(my_details_->details);
  }

  NodeDetails my_details() const {
    return my_details_;
  }
  
private:
  client_register_type register_;  
  network_manager_type manager_;
  
  NodeDetails my_details_;
};

}
}

#endif

