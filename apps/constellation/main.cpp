#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/commandline/params.hpp"
#include "core/macros.hpp"
#include "network/adapters.hpp"
#include "network/management/network_manager.hpp"

#include "constellation.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct CommandLineArguments
{
  using string_list_type  = std::vector<std::string>;
  using peer_list_type    = fetch::Constellation::peer_list_type;
  using adapter_list_type = fetch::network::Adapter::adapter_list_type;

  static const std::size_t DEFAULT_NUM_LANES     = 4;
  static const std::size_t DEFAULT_NUM_EXECUTORS = DEFAULT_NUM_LANES;
  static const uint16_t    DEFAULT_PORT          = 8000;

  uint16_t       port{0};
  peer_list_type peers;
  std::size_t    num_executors;
  std::size_t    num_lanes;
  std::string    interface;
  std::string    dbdir;

  static CommandLineArguments Parse(int argc, char **argv,
                                    adapter_list_type const &adapters)
  {
    CommandLineArguments args;

    // define the parameters
    std::string raw_peers;

    fetch::commandline::Params parameters;
    parameters.add(args.port, "port", "The starting port for ledger services",
                   DEFAULT_PORT);
    parameters.add(args.num_executors, "executors",
                   "The number of executors to configure",
                   DEFAULT_NUM_EXECUTORS);
    parameters.add(args.num_lanes, "lanes", "The number of lanes to be used",
                   DEFAULT_NUM_LANES);
    parameters.add(
        raw_peers, "peers",
        "The comma separated list of addresses to initially connect to",
        std::string{});
    parameters.add(args.dbdir, "db-prefix",
                   "The directory or prefix added to the node storage",
                   std::string{"node_storage"});

    std::size_t const num_adapters = adapters.size();
    if (num_adapters == 0)
    {
      throw std::runtime_error("Unable to detect system network interfaces");
    }
    else if (num_adapters == 1)
    {
      args.interface = adapters[0].address().to_string();
    }
    else
    {
      parameters.add(args.interface, "interface",
                     "The network interface to used for public communication");
    }

    // parse the args
    parameters.Parse(argc, const_cast<char const **>(argv));

    // update the peers
    args.SetPeers(raw_peers);

    // validate the interface
    bool valid_adapter = false;
    for (auto const &adapter : adapters)
    {
      if (adapter.address().to_string() == args.interface)
      {
        valid_adapter = true;
        break;
      }
    }

    // handle invalid interface address
    if (!valid_adapter)
    {
      throw std::runtime_error("Invalid interface address");
    }

    return args;
  }

  void SetPeers(std::string const &raw_peers)
  {

    // split the peers
    std::size_t          position = 0;
    fetch::network::Peer peer;
    while (std::string::npos != position)
    {

      // locate the separator
      std::size_t const separator_position = raw_peers.find(',', position);

      // parse the peer
      std::string const peer_address =
          raw_peers.substr(position, separator_position);
      if (peer.Parse(peer_address))
      {
        peers.push_back(peer);
      }
      else
      {
        fetch::logger.Warn("Failed to parse input peer address: ",
                           peer_address);
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

  friend std::ostream &operator<<(
      std::ostream &s, CommandLineArguments const &args) FETCH_MAYBE_UNUSED
  {
    s << "db-prefix: " << args.dbdir << std::endl;
    s << "port.....: " << args.port << std::endl;
    s << "peers....: ";
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

    auto const network_adapters = fetch::network::Adapter::GetAdapters();

    auto const args = CommandLineArguments::Parse(argc, argv, network_adapters);

    fetch::logger.Info("Configuration:\n", args);

    // create and run the constellation
    auto constellation = fetch::Constellation::Create(
        args.port, args.num_executors, args.num_lanes,
        fetch::Constellation::DEFAULT_NUM_SLICES, args.dbdir);
    constellation->Run(args.peers);

    exit_code = EXIT_SUCCESS;
  }
  catch (std::exception &ex)
  {
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  }

  return exit_code;
}
