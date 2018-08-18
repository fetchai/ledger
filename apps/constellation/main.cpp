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

#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/commandline/params.hpp"
#include "core/json/document.hpp"
#include "core/macros.hpp"
#include "core/script/variant.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "network/adapters.hpp"
#include "network/fetch_asio.hpp"
#include "network/management/network_manager.hpp"

#include "bootstrap_monitor.hpp"
#include "constellation.hpp"

#include <array>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {

using Prover       = fetch::crypto::Prover;
using BootstrapPtr = std::unique_ptr<fetch::BootstrapMonitor>;
using ProverPtr    = std::unique_ptr<Prover>;

struct CommandLineArguments
{
  using string_list_type  = std::vector<std::string>;
  using peer_list_type    = fetch::Constellation::peer_list_type;
  using adapter_list_type = fetch::network::Adapter::adapter_list_type;

  static const std::size_t DEFAULT_NUM_LANES     = 4;
  static const std::size_t DEFAULT_NUM_SLICES    = 4;
  static const std::size_t DEFAULT_NUM_EXECUTORS = DEFAULT_NUM_LANES;
  static const uint16_t    DEFAULT_PORT          = 8000;
  static const uint32_t    DEFAULT_NETWORK_ID    = 0x10;

  uint16_t       port{0};
  uint32_t       network_id;
  peer_list_type peers;
  std::size_t    num_executors;
  std::size_t    num_lanes;
  std::size_t    num_slices;
  std::string    interface;
  bool           bootstrap{false};
  std::string    dbdir;

  static CommandLineArguments Parse(int argc, char **argv, BootstrapPtr &bootstrap,
                                    Prover const &prover)
  {
    CommandLineArguments args;

    // define the parameters
    std::string raw_peers;

    fetch::commandline::Params parameters;
    std::string                external_address;
    parameters.add(args.port, "port", "The starting port for ledger services", DEFAULT_PORT);
    parameters.add(args.num_executors, "executors", "The number of executors to configure",
                   DEFAULT_NUM_EXECUTORS);
    parameters.add(args.num_lanes, "lanes", "The number of lanes to be used", DEFAULT_NUM_LANES);
    parameters.add(args.num_lanes, "slices", "The number of slices to be used", DEFAULT_NUM_SLICES);
    parameters.add(raw_peers, "peers",
                   "The comma separated list of addresses to initially connect to", std::string{});
    parameters.add(args.dbdir, "db-prefix", "The directory or prefix added to the node storage",
                   std::string{"node_storage"});
    parameters.add(args.network_id, "network-id", "The network id", DEFAULT_NETWORK_ID);
    parameters.add(args.interface, "interface", "The network id", std::string{"127.0.0.1"});
    parameters.add(external_address, "bootstrap", "Enable bootstrap network support",
                   std::string{});

    // parse the args
    parameters.Parse(argc, argv);

    // update the peers
    args.SetPeers(raw_peers);

    args.bootstrap = (!external_address.empty());
    if (args.bootstrap)
    {
      // create the boostrap node
      bootstrap =
          std::make_unique<fetch::BootstrapMonitor>(prover.identity(), args.port, args.network_id);

      // augment the peer list with the bootstrapped version
      if (bootstrap->Start(args.peers))
      {
        args.interface = bootstrap->external_address();
      }
    }

    return args;
  }

  void SetPeers(std::string const &raw_peers)
  {
    if (!raw_peers.empty())
    {
      // split the peers
      std::size_t          position = 0;
      fetch::network::Peer peer;
      while (std::string::npos != position)
      {
        // locate the separator
        std::size_t const separator_position = raw_peers.find(',', position);

        // parse the peer
        std::string const peer_address = raw_peers.substr(position, separator_position);
        if (peer.Parse(peer_address))
        {
          peers.push_back(peer);
        }
        else
        {
          fetch::logger.Warn("Failed to parse input peer address: '", peer_address, "'");
        }

        // update the position for the next search
        if (std::string::npos == separator_position)
        {
          position = std::string::npos;
        }
        else
        {
          position = separator_position + 1;  // advance past separator
        }
      }
    }
  }

  friend std::ostream &operator<<(std::ostream &              s,
                                  CommandLineArguments const &args) FETCH_MAYBE_UNUSED
  {
    s << "port...........: " << args.port << std::endl;
    s << "network id.....: 0x" << std::hex << args.network_id << std::dec << std::endl;
    s << "num executors..: " << args.num_executors << std::endl;
    s << "num lanes......: " << args.num_lanes << std::endl;
    s << "bootstrap......: " << args.bootstrap << std::endl;
    s << "db-prefix......: " << args.dbdir << std::endl;
    s << "interface......: " << args.interface << std::endl;
    s << "peers..........: ";
    for (auto const &peer : args.peers)
    {
      s << peer << ' ';
    }
    s << '\n';
    return s;
  }
};

ProverPtr GenereateP2PKey()
{
  fetch::crypto::ECDSASigner *certificate = new fetch::crypto::ECDSASigner();
  certificate->GenerateKeys();

  return ProverPtr{certificate};
}

}  // namespace

int main(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;

  fetch::commandline::DisplayCLIHeader("Constellation");

  try
  {
    // create and load the main certificate for the bootstrapper
    ProverPtr p2p_key = GenereateP2PKey();

    BootstrapPtr bootstrap_monitor;
    auto const   args = CommandLineArguments::Parse(argc, argv, bootstrap_monitor, *p2p_key);

    fetch::logger.Info("Configuration:\n", args);

    // create and run the constellation
    auto constellation = std::make_unique<fetch::Constellation>(
        std::move(p2p_key), args.port, args.num_executors, args.num_lanes, args.num_slices,
        args.interface, args.dbdir);

    // run the application
    constellation->Run(args.peers);

    // stop the bootstrapper if we have one
    if (bootstrap_monitor)
    {
      bootstrap_monitor->Stop();
    }

    exit_code = EXIT_SUCCESS;
  }
  catch (std::exception &ex)
  {
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  }

  return exit_code;
}
