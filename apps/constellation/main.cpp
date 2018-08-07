#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/commandline/params.hpp"
#include "core/macros.hpp"
#include "core/script/variant.hpp"
#include "core/json/document.hpp"
#include "network/adapters.hpp"
#include "network/management/network_manager.hpp"
#include "network/fetch_asio.hpp"

#include "constellation.hpp"

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include <system_error>

namespace {

struct CommandLineArguments
{
  using string_list_type  = std::vector<std::string>;
  using peer_list_type    = fetch::Constellation::peer_list_type;
  using adapter_list_type = fetch::network::Adapter::adapter_list_type;

  static const std::size_t DEFAULT_NUM_LANES     = 4;
  static const std::size_t DEFAULT_NUM_EXECUTORS = DEFAULT_NUM_LANES;
  static const uint16_t    DEFAULT_PORT          = 8000;
  static const uint32_t    DEFAULT_NETWORK_ID    = 0x10;

  uint16_t       port{0};
  uint32_t       network_id;
  peer_list_type peers;
  std::size_t    num_executors;
  std::size_t    num_lanes;
  std::string    interface;
  std::string    dbdir;

  static CommandLineArguments Parse(int argc, char **argv, adapter_list_type const &adapters)
  {
    CommandLineArguments args;

    // define the parameters
    std::string raw_peers;
    bool bootstrap;

    fetch::commandline::Params parameters;
    parameters.add(args.port, "port", "The starting port for ledger services", DEFAULT_PORT);
    parameters.add(args.num_executors, "executors", "The number of executors to configure",
                   DEFAULT_NUM_EXECUTORS);
    parameters.add(args.num_lanes, "lanes", "The number of lanes to be used", DEFAULT_NUM_LANES);
    parameters.add(raw_peers, "peers",
                   "The comma separated list of addresses to initially connect to", std::string{});
    parameters.add(args.dbdir, "db-prefix", "The directory or prefix added to the node storage",
                   std::string{"node_storage"});
    parameters.add(args.network_id, "network-id", "The network id", DEFAULT_NETWORK_ID);
    parameters.add(bootstrap, "bootstrap", "Enable bootstrap network support", false);

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

    // if we have been configured to do so then bootstrap
    if (bootstrap)
    {
      args.Bootstrap();
    }

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

  void Bootstrap()
  {
    static constexpr std::size_t BUFFER_SIZE = 1024;

    using IoService = asio::io_service;
    using Resolver = asio::ip::tcp::resolver;
    using Socket = asio::ip::tcp::socket;

    IoService io_service{};
    Resolver resolver(io_service);

    Resolver::query query("35.188.32.73", std::to_string(10000));

    Resolver::iterator endpoint = resolver.resolve(query);

    std::error_code ec{};
    Socket socket(io_service);
    if (endpoint != Resolver::iterator{}) {
      socket.connect(*endpoint, ec);

      if (ec)
      {
        std::cerr << "Failed to connect to boostrap node: " << ec.message() << std::endl;
        return;
      }

      // create the request
      fetch::script::Variant variant;
      variant.MakeObject();
      variant["port"] = port + fetch::Constellation::P2P_PORT_OFFSET;
      variant["network-id"] = network_id;

      // format the request
      std::string request;
      {
        std::ostringstream oss;
        oss << variant;
        request = oss.str();
      }

      socket.write_some(asio::buffer(request), ec);

      if (ec)
      {
        std::cerr << "Failed to send boostrap request: " << ec.message() << std::endl;
        return;
      }

      fetch::byte_array::ByteArray buffer;
      buffer.Resize(BUFFER_SIZE);

      std::size_t const num_bytes = socket.read_some(asio::buffer(buffer.pointer(), buffer.size()), ec);

      if (ec)
      {
        std::cerr << "Failed to recv the response from the server: " << ec.message() << std::endl;
        return;
      }

      if (num_bytes > 0)
      {
        buffer.Resize(num_bytes);

        std::cout << buffer << std::endl;
      }
      else
      {
        std::cerr << "Didn't recv any more data" << std::endl;
        return;
      }

      // try and parse the json documnet
      fetch::json::JSONDocument doc;
      doc.Parse(buffer);

      // check the formatting of the response
      auto const &root = doc.root();
      if (!root.is_object())
      {
        std::cerr << "Incorrect formatting (object)" << std::endl;
        return;
      }

      auto const &peer_list = root["peers"];
      if (peer_list.is_undefined())
      {
        std::cerr << "Incorrect formatting (undefined)" << std::endl;
        return;
      }

      if (!peer_list.is_array())
      {
        std::cerr << "Incorrect formatting (array)" << std::endl;
        return;
      }

      fetch::network::Peer peer;
      for (std::size_t i = 0; i < peer_list.size(); ++i)
      {
        std::string const peer_address = peer_list[i].As<std::string>();
        if (peer.Parse(peer_address))
        {
          peers.push_back(peer);
        }
        else
        {
          std::cerr << "Failed to parse address: " << peer_address << std::endl;
        }
      }
    }
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
      std::string const peer_address = raw_peers.substr(position, separator_position);
      if (peer.Parse(peer_address))
      {
        peers.push_back(peer);
      }
      else
      {
        fetch::logger.Warn("Failed to parse input peer address: ", peer_address);
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

  friend std::ostream &operator<<(std::ostream &              s,
                                  CommandLineArguments const &args) FETCH_MAYBE_UNUSED
  {
    s << "port...........: " << args.port << std::endl;
    s << "network id.....: 0x" << std::hex << args.network_id << std::dec << std::endl;
    s << "num executors..: " << args.num_executors << std::endl;
    s << "num lanes......: " << args.num_lanes << std::endl;
    s << "interface......: " << args.interface << std::endl;
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

    auto const network_adapters = fetch::network::Adapter::GetAdapters();

    auto const args = CommandLineArguments::Parse(argc, argv, network_adapters);

    fetch::logger.Info("Configuration:\n", args);

    // create and run the constellation
    auto constellation =
        fetch::Constellation::Create(args.port, args.num_executors, args.num_lanes,
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
