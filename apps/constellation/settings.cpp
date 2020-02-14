//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "settings.hpp"

#include "core/commandline/parameter_parser.hpp"
#include "logging/logging.hpp"
#include "vectorise/platform.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <thread>

namespace fetch {
namespace {

const uint32_t DEFAULT_NUM_LANES          = 1;
const uint32_t DEFAULT_NUM_SLICES         = 500;
const uint32_t DEFAULT_NUM_EXECUTORS      = DEFAULT_NUM_LANES;
const uint16_t DEFAULT_PORT               = 8000;
const uint16_t DEFAULT_MESSENGER_PORT     = 9010;
const uint32_t DEFAULT_BLOCK_INTERVAL     = 0;  // milliseconds - zero means no mining
const uint32_t DEFAULT_CABINET_SIZE       = 10;
const uint32_t DEFAULT_STAKE_DELAY_PERIOD = 5;
const uint32_t DEFAULT_AEON_PERIOD        = 25;
const uint32_t DEFAULT_MAX_PEERS          = 3;
const uint32_t DEFAULT_TRANSIENT_PEERS    = 1;
const uint32_t NUM_SYSTEM_THREADS = static_cast<uint32_t>(std::thread::hardware_concurrency());

}  // namespace

// clang-format off
/**
 * Construct the settings object
 */
Settings::Settings()
  : num_lanes             {*this, "lanes",                   DEFAULT_NUM_LANES,            "Number of lanes to be used"}
  , num_slices            {*this, "slices",                  DEFAULT_NUM_SLICES,           "Number of slices to be used"}
  , block_interval        {*this, "block-interval",          DEFAULT_BLOCK_INTERVAL,       "Block interval is milliseconds"}
  , standalone            {*this, "standalone",              false,                        "Whether the network should run in standalone mode"}
  , private_network       {*this, "private-network",         false,                        "Whether the network should run as part of a private network"}
  , initial_address       {*this, "initial-address",         "",                           "The initial address where all funds can be found for a standalone node"}
  , db_prefix             {*this, "db-prefix",               "node_storage",               "Filename prefix for constellation databases"}
  , port                  {*this, "port",                    DEFAULT_PORT,                 "Starting port for ledger services"}
  , peers                 {*this, "peers",                   {},                           "Comma-separated list of addresses to initially connect to"}
  , external              {*this, "external",                "127.0.0.1",                  "This node's global IP address or hostname"}
  , config                {*this, "config",                  "",                           "Path to the manifest configuration"}
  , max_peers             {*this, "max-peers",               DEFAULT_MAX_PEERS,            "Max number of peers to connect to"}
  , transient_peers       {*this, "transient-peers",         DEFAULT_TRANSIENT_PEERS,      "Number of peers to randomly choose from for answers sent to peer requests"}
  , peer_update_interval  {*this, "peers-update-cycle-ms",   0,                            "Peering updates delay"}
  , disable_signing       {*this, "disable-signing",         false,                        "Disable signing all network messages"}
  , kademlia_routing      {*this, "kademlia-routing",        true,                         "Whether kademlia routing should be used in the main P2P network"}
  , bootstrap             {*this, "bootstrap",               false,                        "Whether we should connect to the bootstrap server"}
  , discoverable          {*this, "discoverable",            false,                        "Whether this node can be advertised on the bootstrap server"}
  , hostname              {*this, "host-name",               "",                           "Hostname or identifier for this node"}
  , network_name          {*this, "network",                 "",                           "Name of the bootstrap network to connect to"}
  , token                 {*this, "token",                   "",                           "Authentication token when talking to bootstrap"}
  , num_processor_threads {*this, "processor-threads",       NUM_SYSTEM_THREADS,           "Number of processor threads"}
  , num_verifier_threads  {*this, "verifier-threads",        NUM_SYSTEM_THREADS,           "Number of verifier threads"}
  , num_executors         {*this, "executors",               DEFAULT_NUM_EXECUTORS,        "Number of transaction executors"}
  , genesis_file_location {*this, "genesis-file-location",   "",                           "Path to the genesis file (usually genesis_file.json)"}
  , experimental_features {*this, "experimental",            {},                           "Comma-separated list of experimental features to enable"}
  , proof_of_stake        {*this, "pos",                     false,                        "Enable Proof-of-Stake consensus"}
  , max_cabinet_size      {*this, "max-cabinet-size",        DEFAULT_CABINET_SIZE,         "Maximum cabinet size"}
  , stake_delay_period    {*this, "stake-delay-period",      DEFAULT_STAKE_DELAY_PERIOD,   "<deprecated>"}
  , aeon_period           {*this, "aeon-period",             DEFAULT_AEON_PERIOD,          "Number of blocks each cabinet is governing"}
  , graceful_failure      {*this, "graceful-failure",        false,                        "Whether to shutdown on critical system failures"}
  , fault_tolerant        {*this, "fault-tolerant",          false,                        "Whether to crash on critical system failures"}
  , enable_agents         {*this, "enable-agents",           false,                        "Run the node with agent support"}
  , messenger_port        {*this, "messenger-port",          DEFAULT_MESSENGER_PORT,       "Port agents connect to"}
{}
// clang-format on

/**
 * Given the specified command line arguments, update the settings from the cmd line and the
 * environment
 *
 * @param argc The number of command line arguments
 * @param argv The array of command line arguments
 */
bool Settings::Update(int argc, char **argv)
{
  UpdateFromEnv("CONSTELLATION_");
  return UpdateFromArgs(argc, argv) && Validate();
}

/**
 * Display the summary of all the settings
 *
 * @param stream The output stream to populate
 * @param settings The reference to the settings object
 * @return The output stream that has been populated
 */
std::ostream &operator<<(std::ostream &stream, Settings const &settings)
{
  // First pass determine the max settings length
  std::size_t setting_name_length{0};
  for (auto const *setting : settings.settings())
  {
    setting_name_length = std::max(setting_name_length, setting->name().size());
  }

  // Send pass display the settings
  for (auto const *setting : settings.settings())
  {
    auto const &      name    = setting->name();
    std::size_t const padding = setting_name_length - name.size();

    // write name + padding
    stream << name;
    for (std::size_t i = 0; i < padding; ++i)
    {
      stream << '.';
    }
    stream << ": ";

    // write the value to the stream
    setting->ToStream(stream);
    stream << '\n';
  }

  return stream;
}

/**
 * Internal: Validate that all the parameters are correctly set
 *
 * @return true if the configuration is valid, otherwise false
 */
bool Settings::Validate()
{
  bool valid{true};

  // network mode checks
  if (standalone.value() && private_network.value())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Can not have both the -standalone and -private-network flags");
    valid = false;
  }

  // number of lanes check
  if (!platform::IsLog2(num_lanes.value()))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "The number of lanes needs to be a valid power of 2");
    valid = false;
  }

  return valid;
}

}  // namespace fetch
