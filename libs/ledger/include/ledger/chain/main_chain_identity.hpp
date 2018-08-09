#pragma once
#include <utility>

#include "ledger/chain/main_chain_details.hpp"
#include "network/management/connection_register.hpp"
#include "network/service/client.hpp"

namespace fetch {
namespace chain {

class MainChainIdentity
{
public:
  using connectivity_details_type = MainChainDetails;
  using client_register_type      = fetch::network::ConnectionRegister<connectivity_details_type>;
  using network_manager_type      = fetch::network::NetworkManager;
  using mutex_type                = fetch::mutex::Mutex;
  using connection_handle_type    = client_register_type::connection_handle_type;
  using ping_type                 = uint32_t;

  enum
  {
    PING_MAGIC = 1337
  };

  MainChainIdentity(client_register_type reg, network_manager_type const &nm)
    : register_(std::move(reg)), manager_(nm)
  {}

  /// External controls
  /// @{
  ping_type Ping() { return PING_MAGIC; }

  void Hello(connection_handle_type const &client)
  {
    auto details     = register_.GetDetails(client);
    details->is_peer = true;
  }

  void AuthenticateController(connection_handle_type const &client)
  {
    auto details = register_.GetDetails(client);
    {
      std::lock_guard<mutex_type> lock(*details);
      details->is_controller = true;
    }
  }

  /// @}

private:
  client_register_type register_;
  network_manager_type manager_;
};

}  // namespace chain
}  // namespace fetch
