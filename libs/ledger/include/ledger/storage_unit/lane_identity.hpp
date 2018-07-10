#ifndef LEDGER_STORAGE_UNIT_LANE_IDENTITY_HPP
#define LEDGER_STORAGE_UNIT_LANE_IDENTITY_HPP
#include"network/service/client.hpp"
#include"network/tcp/connection_register.hpp"
#include"network/service/client.hpp"
#include"ledger/storage_unit/lane_connectivity_details.hpp"

namespace fetch
{
namespace ledger
{

class LaneIdentity
{
public:
  using connectivity_details_type = LaneConnectivityDetails;  
  using client_register_type = fetch::network::ConnectionRegister< connectivity_details_type >;
  using network_manager_type = fetch::network::ThreadManager;
  using mutex_type = fetch::mutex::Mutex;
  using connection_handle_type = client_register_type::connection_handle_type;
  using ping_type = uint32_t;
  using lane_type = uint32_t;  
  
  enum {
    PING_MAGIC = 1337
  };
      
  
  LaneIdentity(client_register_type reg, network_manager_type nm )
    : register_(reg), manager_(nm)
  {
    lane_ = uint32_t(-1);
  }

  /// External controls
  /// @{
  ping_type Ping() 
  {
    return PING_MAGIC;
  }

  void Hello(connection_handle_type const &client) 
  {    
    auto details = register_.GetDetails(client);
    details->is_peer = true;
  }

  
  void AuthenticateController(connection_handle_type const &client) 
  {
    auto details = register_.GetDetails(client);
    {
      std::lock_guard< mutex_type > lock( *details );
      details->is_controller = true;
    }
  }
  
  
  lane_type GetLaneNumber() 
  {
    return lane_;
  }

  /// @}

  /// Internal controls
  /// @{
  void SetLaneNumber(lane_type const &lane) 
  {
    lane_ = lane;
  }
  
  
  /// @}
private:
  client_register_type register_;
  network_manager_type manager_;

  std::atomic< lane_type > lane_;      
};

}
}

#endif
