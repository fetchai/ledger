#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/commandline/params.hpp"
#include "core/macros.hpp"
#include "core/script/variant.hpp"
#include "core/json/document.hpp"
#include "network/adapters.hpp"
#include "network/management/network_manager.hpp"
#include "network/fetch_asio.hpp"

#include "bootstrap_monitor.hpp"
#include "constellation.hpp"

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include <system_error>

namespace {

using BootstrapPtr = std::unique_ptr<fetch::BootstrapMonitor>;

struct CommandLineArguments
{
  using string_list_type  = std::vector<std::string>;
  using peer_list_type    = fetch::Constellation::peer_list_type;
  using adapter_list_type = fetch::network::Adapter::adapter_list_type;

  static const std::size_t DEFAULT_NUM_LANES = 4;
  static const std::size_t DEFAULT_NUM_SLICES    = 4;
  static const std::size_t DEFAULT_NUM_EXECUTORS = DEFAULT_NUM_LANES;
  static const uint16_t DEFAULT_PORT = 8000;
  static const uint32_t DEFAULT_NETWORK_ID = 0x10;

  uint16_t port{0};
  uint32_t network_id;
  peer_list_type peers;
  std::size_t num_executors;
  std::size_t num_lanes;
  std::size_t    num_slices;
  bool bootstrap{false};
  std::string dbdir;


  static CommandLineArguments Parse(int argc, char **argv, BootstrapPtr &bootstrap)
  {
    CommandLineArguments args;

    // define the parameters
    std::string raw_peers;

    fetch::commandline::Params parameters;
    std::string external_address;
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
    parameters.add(external_address, "bootstrap", "Enable bootstrap network support",
                   std::string{});

    // parse the args
    parameters.Parse(argc, const_cast<char const **>(argv));

    // update the peers
    args.SetPeers(raw_peers);

    args.bootstrap = (!external_address.empty());
    if (args.bootstrap)
    {
      // create the boostrap node
      bootstrap = std::make_unique<fetch::BootstrapMonitor>(
        args.port,
        args.network_id,
        external_address);

      // augment the peer list with the bootstrapped version
      bootstrap->Start(args.peers);
    }

    return args;
  }

  void SetPeers(std::string const &raw_peers)
  {
    if (!raw_peers.empty())
    {
      // split the peers
      std::size_t position = 0;
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

  friend std::ostream &operator<<(std::ostream &s,
                                  CommandLineArguments const &args) FETCH_MAYBE_UNUSED
  {
    s << "port...........: " << args.port << std::endl;
    s << "network id.....: 0x" << std::hex << args.network_id << std::dec << std::endl;
    s << "num executors..: " << args.num_executors << std::endl;
    s << "num lanes......: " << args.num_lanes << std::endl;
    s << "bootstrap......: " << args.bootstrap << std::endl;
    s << "db-prefix......: " << args.dbdir << std::endl;
    s << "peers..........: ";
    for (auto const &peer : args.peers)
    {
      s << peer << ' ';
    }
    s << '\n';
    return s;
  }
};

}  // namespace

int main(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;

  fetch::commandline::DisplayCLIHeader("Constellation");

  try
  {
    BootstrapPtr bootstrap_monitor;

    auto const args = CommandLineArguments::Parse(argc, argv, bootstrap_monitor);

    fetch::logger.Info("Configuration:\n", args);

    // create and run the constellation
    auto constellation = fetch::Constellation::Create(args.port, args.num_executors, args.num_lanes,
                                                      args.num_slices, args.dbdir);

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
