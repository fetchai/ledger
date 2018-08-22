#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <utility>

#include "ledger/chain/main_chain_details.hpp"
#include "network/generics/shared_with_lock.hpp"
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

  static constexpr char const *LOGGING_NAME = "MainChainIdentity";

  enum
  {
    PING_MAGIC = 1337
  };

  MainChainIdentity(client_register_type reg, network_manager_type const &nm,
                    generics::SharedWithLock<MainChainDetails> my_details)
    : register_(std::move(reg)), manager_(nm), my_details_(my_details)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME,"MainChainIdentity::MainChainIdentity ", bool(my_details_));
  }

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

  MainChainDetails ExchangeDetails(connection_handle_type const &client,
                                   MainChainDetails              remote_details)
  {
    auto details = register_.GetDetails(client);
    if (details)
    {
      std::lock_guard<mutex_type> lock(*details);
      details->CopyFromRemotePeer(remote_details);
    }
    else
    {
      FETCH_LOG_DEBUG(LOGGING_NAME,"MainChainIdentity::ExchangeDetails: NO LOCAL DETAILS HELD!");
    }

    MainChainDetails my_details_copy;
    my_details_.CopyOut(my_details_copy);
    return my_details_copy;
  }

  /// @}

private:
  client_register_type                       register_;
  network_manager_type                       manager_;
  generics::SharedWithLock<MainChainDetails> my_details_;
};

}  // namespace chain
}  // namespace fetch
