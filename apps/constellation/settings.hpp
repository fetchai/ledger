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

#include "core/feature_flags.hpp"
#include "network/uri.hpp"
#include "settings/setting.hpp"
#include "settings/setting_collection.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace fetch {

/**
 * Command line / Environment variable settings
 */
class Settings : private settings::SettingCollection
{
public:
  using PeerList = std::vector<network::Uri>;

  // Construction / Destruction
  Settings();
  Settings(Settings const &) = delete;
  Settings(Settings &&)      = delete;
  ~Settings()                = default;

  bool Update(int argc, char **argv);

  /// @name High Level Network Settings
  /// @{
  settings::Setting<uint32_t> num_lanes;
  settings::Setting<uint32_t> num_slices;
  settings::Setting<uint32_t> block_interval;
  /// @}

  /// @name Network Mode
  /// @{
  settings::Setting<bool> standalone;
  settings::Setting<bool> private_network;
  /// @}

  /// @name Shards
  /// @{
  settings::Setting<std::string> db_prefix;
  /// @}

  /// @name Networking / P2P Manifest
  /// @{
  settings::Setting<uint16_t>    port;
  settings::Setting<PeerList>    peers;
  settings::Setting<std::string> external;
  settings::Setting<std::string> config;
  settings::Setting<uint32_t>    max_peers;
  settings::Setting<uint32_t>    transient_peers;
  settings::Setting<uint32_t>    peer_update_interval;
  settings::Setting<bool>        disable_signing;
  /// @}

  /// @name Bootstrap Config
  /// @{
  settings::Setting<bool>        bootstrap;
  settings::Setting<bool>        discoverable;
  settings::Setting<std::string> hostname;
  settings::Setting<std::string> network_name;
  settings::Setting<std::string> token;
  /// @}

  /// @name Threading Settings
  /// @{
  settings::Setting<uint32_t> num_processor_threads;
  settings::Setting<uint32_t> num_verifier_threads;
  settings::Setting<uint32_t> num_executors;
  /// @}

  /// @name State File
  /// @{
  settings::Setting<bool> dump_state;
  settings::Setting<bool> load_state;
  /// @}

  /// @name Experimental
  /// @{
  settings::Setting<core::FeatureFlags> experimental_features;
  /// @}

  /// @name Proof of Stake
  /// @{
  settings::Setting<bool>        proof_of_stake;
  settings::Setting<std::string> beacon_address;
  /// @}

  // Operators
  Settings &operator=(Settings const &) = delete;
  Settings &operator=(Settings &&) = delete;

  friend std::ostream &operator<<(std::ostream &stream, Settings const &settings);

private:
  bool Validate();
};

}  // namespace fetch
