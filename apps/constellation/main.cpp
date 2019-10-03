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

#include "bootstrap_monitor.hpp"
#include "config_builder.hpp"
#include "constants.hpp"
#include "constellation.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/commandline/params.hpp"
#include "core/logging.hpp"
#include "core/macros.hpp"
#include "core/runnable.hpp"
#include "core/string/to_lower.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/fetch_identity.hpp"
#include "crypto/identity.hpp"
#include "crypto/key_generator.hpp"
#include "crypto/prover.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/shards/manifest.hpp"
#include "network/adapters.hpp"
#include "network/peer.hpp"
#include "network/uri.hpp"
#include "settings.hpp"
#include "version/cli_header.hpp"
#include "version/fetch_version.hpp"

#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

constexpr char const *LOGGING_NAME = "main";

using fetch::Settings;
using fetch::crypto::Prover;
using fetch::Constellation;
using fetch::BootstrapMonitor;
using fetch::core::WeakRunnable;
using fetch::network::Uri;

using BootstrapPtr = std::unique_ptr<BootstrapMonitor>;
using ProverPtr    = std::shared_ptr<Prover>;
using NetworkMode  = fetch::Constellation::NetworkMode;
using UriSet       = fetch::Constellation::UriSet;
using Uris         = std::vector<Uri>;

std::atomic<fetch::Constellation *> gConstellationInstance{nullptr};
std::atomic<std::size_t>            gInterruptCount{0};

UriSet ToUriSet(Uris const &uris)
{
  UriSet s{};
  for (auto const &uri : uris)
  {
    s.emplace(uri);
  }

  return s;
}

/**
 * The main interrupt handler for the application
 */
void InterruptHandler(int /*signal*/)
{
  std::size_t const interrupt_count = ++gInterruptCount;

  if (interrupt_count > 1)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "User requests stop of service (count: ", interrupt_count, ")");
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "User requests stop of service");
  }

  if (gConstellationInstance != nullptr)
  {
    gConstellationInstance.load()->SignalStop();
  }

  if (interrupt_count >= 3)
  {
    std::exit(1);
  }
}

/**
 * Determine is a version flag has been present on the command line
 *
 * @param argc The number of args
 * @param argv The array of arguments
 * @return true if the version flag is present, otherwise false
 */
bool HasVersionFlag(int argc, char **argv)
{
  static const std::string FULL_VERSION_FLAG{"--version"};
  static const std::string SHORT_VERSION_FLAG{"-v"};

  bool has_version{false};

  for (int i = 1; i < argc; ++i)
  {
    if ((FULL_VERSION_FLAG == argv[i]) || (SHORT_VERSION_FLAG == argv[i]))
    {
      has_version = true;
      break;
    }
  }

  return has_version;
}

/**
 * Based on the settings create a bootstrap instance if necessary
 *
 * @param settings The settings of the system
 * @param prover The key for the node
 * @param uris The initial set of nodes
 * @return The new bootstrap pointer if one exists
 */
BootstrapPtr CreateBootstrap(Settings const &settings, ProverPtr const &prover, UriSet &uris)
{
  BootstrapPtr bootstrap{};

  if (settings.bootstrap.value())
  {
    // build the bootstrap monitor instance
    bootstrap = std::make_unique<BootstrapMonitor>(
        prover, settings.port.value() + fetch::P2P_PORT_OFFSET, settings.network_name.value(),
        settings.discoverable.value(), settings.token.value(), settings.hostname.value());

    // run the discover
    bootstrap->DiscoverPeers(uris, settings.external.value());
  }

  return bootstrap;
}

/**
 * Extract the WeakRunnable from bootstrap so that it can be added to a reactor
 *
 * @param bootstrap The bootstrap instance
 * @return The weak runnable to be added to the reactor
 */
WeakRunnable ExtractRunnable(BootstrapPtr const &bootstrap)
{
  if (bootstrap)
  {
    return bootstrap->GetWeakRunnable();
  }

  return {};
}

}  // namespace

int main(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;

  fetch::crypto::mcl::details::MCLInitialiser();

  // Special case for the version flag
  if (HasVersionFlag(argc, argv))
  {
    std::cout << fetch::version::FULL << std::endl;
    return 0;
  }

  // version header
  fetch::version::DisplayCLIHeader("Constellation");

  if (!fetch::version::VALID)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unsupported version - git working tree is dirty");
  }

  try
  {
    Settings settings{};
    if (!settings.Update(argc, argv))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Invalid configuration");
    }
    else
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Input Configuration:\n", settings);

      // create and load the main certificate for the bootstrapper
      auto p2p_key = fetch::crypto::GenerateP2PKey();

      // create the bootrap monitor (if configued to do so)
      auto initial_peers = ToUriSet(settings.peers.value());
      auto bootstrap     = CreateBootstrap(settings, p2p_key, initial_peers);

      for (auto const &uri : initial_peers)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Initial Peer: ", uri);
      }

      // attempt to build the configuration for constellation
      Constellation::Config cfg = BuildConstellationConfig(settings);

      // create and run the constellation
      auto constellation =
          std::make_unique<fetch::Constellation>(std::move(p2p_key), std::move(cfg));

      // update the instance pointer
      gConstellationInstance = constellation.get();

      // register the signal handlers
      std::signal(SIGINT, InterruptHandler);
      std::signal(SIGTERM, InterruptHandler);

      // run the application
      constellation->Run(initial_peers, ExtractRunnable(bootstrap));

      exit_code = EXIT_SUCCESS;
    }
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Fatal Error: ", ex.what());
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  }

  return exit_code;
}
