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

#include "bootstrap_monitor.hpp"
#include "chain/address.hpp"
#include "config_builder.hpp"
#include "constants.hpp"
#include "constellation/constellation.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/commandline/params.hpp"
#include "core/macros.hpp"
#include "core/runnable.hpp"
#include "core/string/to_lower.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/fetch_identity.hpp"
#include "crypto/identity.hpp"
#include "crypto/key_generator.hpp"
#include "crypto/prover.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "logging/logging.hpp"
#include "network/adapters.hpp"
#include "network/peer.hpp"
#include "network/uri.hpp"
#include "settings.hpp"
#include "shards/manifest.hpp"
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

using fetch::BootstrapMonitor;
using fetch::Settings;
using fetch::core::WeakRunnable;
using fetch::crypto::Prover;
using fetch::network::Uri;
using fetch::shards::ServiceIdentifier;

using BootstrapPtr = std::unique_ptr<BootstrapMonitor>;
using ProverPtr    = std::shared_ptr<Prover>;
using NetworkMode  = fetch::constellation::Constellation::NetworkMode;
using UriSet       = fetch::constellation::Constellation::UriSet;
using Uris         = std::vector<Uri>;
using Config       = fetch::constellation::Constellation::Config;

std::atomic<fetch::constellation::Constellation *> gConstellationInstance{nullptr};
std::atomic<std::size_t>                           gInterruptCount{0};

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
 * The makes sure that segmentation faults become catchable.
 * This serves as a means to make the VM resiliant to malformed modules.
 */
namespace {
std::atomic<bool> shutdown_on_critical_failure{false};
}

void ThrowException(int signal)
{
  FETCH_LOG_ERROR(LOGGING_NAME, "Encountered segmentation fault or floating point exception.");
  ERROR_BACKTRACE;

  if (shutdown_on_critical_failure && (gConstellationInstance != nullptr))
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Shutting down and restarting.");

    // Gracefully shutting down
    gConstellationInstance.load()->SignalStop();
  }

  // Throwing an exception that can be caught by the
  // system to allow it to run until shutdown
  switch (signal)
  {
  case SIGSEGV:
    throw std::runtime_error("Segmentation fault.");
  case SIGFPE:
    throw std::runtime_error("Floating point exception.");
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
BootstrapPtr CreateBootstrap(Settings const &settings, Config const &config,
                             ProverPtr const &prover, UriSet &uris)
{
  BootstrapPtr bootstrap{};

  if (settings.bootstrap.value())
  {
    // lookup the external port from the manifest
    auto const service_it = config.manifest.FindService(ServiceIdentifier::Type::CORE);
    if (service_it == config.manifest.end())
    {
      throw std::runtime_error("Unable to read core entry from service manifest");
    }

    // get the reference to the TCP peer entry for the core service
    auto const &core_service_peer = service_it->second.uri().GetTcpPeer();

    // build the bootstrap monitor instance
    bootstrap = std::make_unique<BootstrapMonitor>(
        prover, core_service_peer.port(), settings.network_name.value(),
        settings.discoverable.value(), settings.token.value(), core_service_peer.address());

    // run the discover
    bootstrap->DiscoverPeers(uris, core_service_peer.address());
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
      // create and load the main certificate for the bootstrapper
      auto p2p_key = fetch::crypto::GenerateP2PKey();

      // attempt to build the configuration for constellation
      fetch::constellation::Constellation::Config cfg = BuildConstellationConfig(settings);

      FETCH_LOG_INFO(LOGGING_NAME, "Configuration:\n", settings, "-\n", cfg);

      // setting policy for critical signals
      shutdown_on_critical_failure = settings.graceful_failure.value();

      // create the bootrap monitor (if configued to do so)
      auto initial_peers = ToUriSet(settings.peers.value());
      auto bootstrap     = CreateBootstrap(settings, cfg, p2p_key, initial_peers);

      for (auto const &uri : initial_peers)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Initial Peer: ", uri);
      }

      // create and run the constellation
      auto constellation =
          std::make_unique<fetch::constellation::Constellation>(std::move(p2p_key), std::move(cfg));

      // update the instance pointer
      gConstellationInstance = constellation.get();

      // register the signal handlers
      std::signal(SIGINT, InterruptHandler);
      std::signal(SIGTERM, InterruptHandler);

      // Making the system resillient to segmentation faults
      if (settings.fault_tolerant.value())
      {
        std::signal(SIGSEGV, ThrowException);
        std::signal(SIGFPE, ThrowException);
      }

      // run the application
      try
      {
        exit_code = (constellation->Run(initial_peers, ExtractRunnable(bootstrap))) ? EXIT_SUCCESS
                                                                                    : EXIT_FAILURE;
      }
      catch (std::exception const &ex)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to run constellation with exception: ", ex.what());
      }
      catch (...)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to run constellation with unknown exception");
      }
    }
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Fatal Error: ", ex.what());
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  }

  FETCH_LOG_INFO(LOGGING_NAME, " - ");

  return exit_code;
}
