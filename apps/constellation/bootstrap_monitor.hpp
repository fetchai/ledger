#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "constellation.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/mutex.hpp"
#include "core/state_machine.hpp"
#include "crypto/identity.hpp"
#include "http/json_client.hpp"
#include "network/fetch_asio.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace fetch {

/**
 * The bootstrap monitor is a simple placeholder implementation for a client to the bootstrap
 * server. It has two main functions namely:
 *
 * - The collection of an initial set of peers to attempt to connect to initially
 * - A periodic phone home in order to update the cached set of peer connections.
 */
class BootstrapMonitor
{
public:
  using UriList   = Constellation::UriList;
  using Prover    = crypto::Prover;
  using ProverPtr = std::shared_ptr<Prover>;
  using Identity  = crypto::Identity;

  // Construction / Destruction
  BootstrapMonitor(ProverPtr entity, uint16_t p2p_port, std::string network_name, bool discoverable,
                   std::string token = std::string{}, std::string host_name = std::string{});
  BootstrapMonitor(BootstrapMonitor const &) = delete;
  BootstrapMonitor(BootstrapMonitor &&)      = delete;
  ~BootstrapMonitor()                        = default;

  bool DiscoverPeers(UriList &peers, std::string const &external_address);

  std::string const &external_address() const
  {
    return external_address_;
  }

  core::WeakRunnable GetWeakRunnable() const
  {
    return state_machine_;
  }

  // Operators
  BootstrapMonitor &operator=(BootstrapMonitor const &) = delete;
  BootstrapMonitor &operator=(BootstrapMonitor &&) = delete;

private:
  enum State
  {
    Notify,
  };

  using ConstByteArray  = byte_array::ConstByteArray;
  using StateMachine    = core::StateMachine<State>;
  using StateMachinePtr = std::shared_ptr<StateMachine>;

  /// @name State Machine states
  /// @{
  State OnNotify();
  /// @}

  /// @name Actions
  /// @{
  bool UpdateExternalAddress();
  bool RunDiscovery(UriList &peers);
  bool NotifyNode();
  /// @}

  static char const *ToString(State state);

  StateMachinePtr   state_machine_;
  ProverPtr const   entity_;
  std::string const network_name_;
  bool const        discoverable_;
  uint16_t const    port_;
  std::string const host_name_;
  std::string const token_;
  std::string       external_address_{};
};

inline char const *BootstrapMonitor::ToString(State state)
{
  char const *text = "Unknown";

  switch (state)
  {
  case State::Notify:
    text = "Notify";
    break;
  }

  return text;
}

}  // namespace fetch
