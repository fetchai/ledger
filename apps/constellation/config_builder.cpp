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

#include "config_builder.hpp"
#include "manifest_builder.hpp"
#include "settings.hpp"
#include "vectorise/platform.hpp"

namespace fetch {
namespace {

/**
 * Determine the network mode based on the settings configuration
 *
 * @param settings The settings of the system
 * @return The calculated nework mode
 */
Constellation::NetworkMode GetNetworkMode(Settings const &settings)
{
  Constellation::NetworkMode mode{Constellation::NetworkMode::PUBLIC_NETWORK};

  if (settings.standalone.value())
  {
    mode = Constellation::NetworkMode::STANDALONE;
  }
  else if (settings.private_network.value())
  {
    mode = Constellation::NetworkMode::PRIVATE_NETWORK;
  }

  return mode;
}

}  // namespace

/**
 * Build the Constellation's configuration based on the settings based in.
 *
 * @param settings The system settings
 * @return The configuration
 */
Constellation::Config BuildConstellationConfig(Settings const &settings)
{
  Constellation::Config cfg;

  BuildManifest(settings, cfg.manifest);
  cfg.log2_num_lanes        = platform::ToLog2(settings.num_lanes.value());
  cfg.num_slices            = settings.num_slices.value();
  cfg.num_executors         = settings.num_executors.value();
  cfg.db_prefix             = settings.db_prefix.value();
  cfg.processor_threads     = settings.num_processor_threads.value();
  cfg.verification_threads  = settings.num_verifier_threads.value();
  cfg.max_peers             = settings.max_peers.value();
  cfg.transient_peers       = settings.transient_peers.value();
  cfg.block_interval_ms     = settings.block_interval.value();
  cfg.peers_update_cycle_ms = settings.peer_update_interval.value();
  cfg.disable_signing       = settings.disable_signing.value();
  cfg.sign_broadcasts       = false;
  cfg.dump_state_file       = settings.dump_state.value();
  cfg.load_state_file       = settings.load_state.value();
  cfg.stakefile_location    = settings.stakefile_location.value();
  cfg.proof_of_stake        = settings.proof_of_stake.value();
  cfg.network_mode          = GetNetworkMode(settings);
  cfg.beacon_address        = settings.beacon_address.value();
  cfg.features              = settings.experimental_features.value();

  return cfg;
}

}  // namespace fetch
