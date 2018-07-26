#ifndef LEDGER_STORAGE_UNIT_LANE_IDENTITY_HPP
#define LEDGER_STORAGE_UNIT_LANE_IDENTITY_HPP
#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "network/management/connection_register.hpp"
#include "network/service/client.hpp"

namespace fetch {
namespace ledger {

class LaneIdentity
{
public:
  using connectivity_details_type = LaneConnectivityDetails;
  using client_register_type =
      fetch::network::ConnectionRegister<connectivity_details_type>;
  using network_manager_type   = fetch::network::NetworkManager;
  using mutex_type             = fetch::mutex::Mutex;
  using connection_handle_type = client_register_type::connection_handle_type;
  using ping_type              = uint32_t;
  using lane_type              = uint32_t;

  enum
  {
    PING_MAGIC = 1337
  };

  LaneIdentity(client_register_type reg, network_manager_type nm,
               crypto::Identity identity)
      : identity_(identity), register_(reg), manager_(nm)
  {
    lane_        = uint32_t(-1);
    total_lanes_ = 0;
  }

  /// External controls
  /// @{
  ping_type Ping() { return PING_MAGIC; }

  crypto::Identity Hello(connection_handle_type const &client,
                         crypto::Identity const &      iden)
  {
    auto details = register_.GetDetails(client);

    if (!details)
    {
      fetch::logger.Error("Failed to find client in client register! ",
                          __FILE__, " ", __LINE__);
      assert(details);
    }
    else
    // TODO: Verify identity if already exists
    {
      std::lock_guard<mutex::Mutex> lock(*details);
      details->identity = iden;
      details->is_peer  = true;
    }

    std::lock_guard<mutex::Mutex> lock(identity_mutex_);
    return identity_;
  }

  void AuthenticateController(connection_handle_type const &client)
  {
    auto details = register_.GetDetails(client);
    {
      std::lock_guard<mutex_type> lock(*details);
      details->is_controller = true;
    }
  }

  crypto::Identity Identity()
  {
    std::lock_guard<mutex::Mutex> lock(identity_mutex_);
    return identity_;
  }

  lane_type GetLaneNumber() { return lane_; }

  lane_type GetTotalLanes() { return total_lanes_; }

  /// @}

  /// Internal controls
  /// @{
  void SetLaneNumber(lane_type const &lane) { lane_ = lane; }

  void SetTotalLanes(lane_type const &t) { total_lanes_ = t; }
  /// @}
  typedef std::function<byte_array::ConstByteArray(
      byte_array::ConstByteArray const &)>
       callable_sign_message_type;
  void OnSignMessage(callable_sign_message_type const &fnc)
  {
    on_sign_message_ = fnc;
  }

private:
  mutex::Mutex               identity_mutex_;
  crypto::Identity           identity_;
  callable_sign_message_type on_sign_message_;

  client_register_type register_;
  network_manager_type manager_;

  std::atomic<lane_type> lane_;
  std::atomic<lane_type> total_lanes_;
};

}  // namespace ledger
}  // namespace fetch

#endif
