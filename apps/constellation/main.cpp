#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/commandline/params.hpp"
#include "network/management/network_manager.hpp"

#include "constellation.hpp"

#include <iostream>
#include <vector>
#include <string>

namespace {

struct CommandLineArguments {
  using string_list_type = std::vector<std::string>;
  using peer_list_type = fetch::Constellation::peer_list_type;

  static const std::size_t DEFAULT_NUM_LANES = 4;
  static const std::size_t DEFAULT_NUM_EXECUTORS = DEFAULT_NUM_LANES;

  uint16_t port{0};
  peer_list_type peers;
  std::size_t num_executors;
  std::size_t num_lanes;

  static CommandLineArguments Parse(int argc, char **argv) {
    CommandLineArguments args;

    // define the parameters
    std::string raw_peers;

    fetch::commandline::Params parameters;
    parameters.add(args.port, "port", "The number of lanes to be configured");
    parameters.add(args.num_executors, "executors", "The number of executors to use", DEFAULT_NUM_EXECUTORS);
    parameters.add(args.num_lanes, "lanes", "The number of lanes to use", DEFAULT_NUM_LANES);
    parameters.add(raw_peers, "peers", "The comma separrated list of addresses to initially connect to", std::string{});

    // parse the args
    parameters.Parse(argc, const_cast<char const **>(argv));

    // update the peers
    args.SetPeers(raw_peers);

    return args;
  }

  void SetPeers(std::string const &raw_peers) {

    // split the peers
    std::size_t position = 0;
    fetch::network::Peer peer;
    while (std::string::npos != position) {

      // locate the separator
      std::size_t const separator_position = raw_peers.find(',', position);

      // parse the peer
      std::string const peer_address = raw_peers.substr(position, separator_position);
      if (peer.Parse(peer_address)) {
        peers.push_back(peer);
      } else {
        fetch::logger.Warn("Failed to parse input peer address: ", peer_address);
      }

      // update the position for the next search
      if (std::string::npos == separator_position) {
        position = std::string::npos;
      } else {
        position = separator_position + 1; // advance past separator
      }
    }
  }

  friend std::ostream &operator<<(std::ostream &s, CommandLineArguments const &args) {
    s << "port...: " << args.port << '\n';
    s << "peers..: ";
    for (auto const &peer : args.peers) {
      s << peer << ' ';
    }
    s << '\n';
    return s;
  }
};

} // namespace

int main(int argc, char **argv) {
  int exit_code = EXIT_FAILURE;

  fetch::commandline::DisplayCLIHeader("Constellation");

  try {

    auto const args = CommandLineArguments::Parse(argc, argv);

    fetch::logger.Info("Configuration:\n", args);

    // create and run the constellation
    auto constellation = fetch::Constellation::Create(
      args.port,
      args.num_executors,
      args.num_lanes
    );
    constellation->Run(args.peers);

    exit_code = EXIT_SUCCESS;
  } catch (std::exception &ex) {
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  }

  return exit_code;
}