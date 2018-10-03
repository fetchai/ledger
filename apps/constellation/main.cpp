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
#include "ledger/metrics/metrics.hpp"
#include "network/adapters.hpp"
#include "network/fetch_asio.hpp"
#include "network/management/network_manager.hpp"

#include "bootstrap_monitor.hpp"
#include "constellation.hpp"

#include <array>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {

static constexpr char const *LOGGING_NAME = "main";

using Prover       = fetch::crypto::Prover;
using BootstrapPtr = std::unique_ptr<fetch::BootstrapMonitor>;
using ProverPtr    = std::unique_ptr<Prover>;

std::atomic<fetch::Constellation *> gConstellationInstance{nullptr};
std::atomic<std::size_t>            gInterruptCount{0};

// REVIEW: Move to platform
uint32_t Log2(uint32_t value)
{
  static constexpr uint32_t VALUE_SIZE_IN_BITS = sizeof(value) << 3;
  return static_cast<uint32_t>(VALUE_SIZE_IN_BITS -
                               static_cast<uint32_t>(__builtin_clz(value) + 1));
}

bool EnsureLog2(uint32_t value)
{
  uint32_t const log2_value = Log2(value);
  return value == (1 << log2_value);
}

struct CommandLineArguments
{
  using StringList  = std::vector<std::string>;
  using UriList     = fetch::Constellation::UriList;
  using AdapterList = fetch::network::Adapter::adapter_list_type;

  static const uint32_t DEFAULT_NUM_LANES     = 4;
  static const uint32_t DEFAULT_NUM_SLICES    = 4;
  static const uint32_t DEFAULT_NUM_EXECUTORS = DEFAULT_NUM_LANES;
  static const uint16_t DEFAULT_PORT          = 8000;
  static const uint32_t DEFAULT_NETWORK_ID    = 0x10;

  uint16_t    port{0};
  uint32_t    network_id;
  UriList     peers;
  uint32_t    num_executors;
  uint32_t    num_lanes;
  uint32_t    log2_num_lanes;
  uint32_t    num_slices;
  std::string interface;
  std::string token;
  bool        bootstrap{false};
  bool        mine{false};
  std::string dbdir;
  std::string external_address;
  std::string host_name;

  static CommandLineArguments Parse(int argc, char **argv, BootstrapPtr &bootstrap,
                                    Prover const &prover)
  {
    CommandLineArguments args;

    // define the parameters
    std::string raw_peers;

    fetch::commandline::Params parameters;
    std::string                bootstrap_address;
    std::string                external_address;
    parameters.add(args.port, "port", "The starting port for ledger services", DEFAULT_PORT);
    parameters.add(args.num_executors, "executors", "The number of executors to configure",
                   DEFAULT_NUM_EXECUTORS);
    parameters.add(args.num_lanes, "lanes", "The number of lanes to be used", DEFAULT_NUM_LANES);
    parameters.add(args.num_slices, "slices", "The number of slices to be used",
                   DEFAULT_NUM_SLICES);
    parameters.add(raw_peers, "peers",
                   "The comma separated list of addresses to initially connect to", std::string{});
    parameters.add(args.dbdir, "db-prefix", "The directory or prefix added to the node storage",
                   std::string{"node_storage"});
    parameters.add(args.network_id, "network-id", "The network id", DEFAULT_NETWORK_ID);
    parameters.add(args.interface, "interface", "The network id", std::string{"127.0.0.1"});
    parameters.add(external_address, "bootstrap", "Enable bootstrap network support",
                   std::string{});
    parameters.add(args.token, "token",
                   "The authentication token to be used with bootstrapping the client",
                   std::string{});
    parameters.add(args.mine, "mine", "Enable mining on this node", false);

    parameters.add(args.external_address, "external", "This node's global IP addr.", std::string{});
    parameters.add(bootstrap_address, "bootstrap", "Src addr for network boostrap.", std::string{});
    parameters.add(args.host_name, "host-name", "The hostname / identifier for this node",
                   std::string{});

    // parse the args
    parameters.Parse(argc, argv);

    // update the peers
    args.SetPeers(raw_peers);

    // ensure that the number lanes is a valid power of 2
    if (!EnsureLog2(args.num_lanes))
    {
      std::cout << "Number of lanes is not a valid log2 number" << std::endl;
      std::exit(1);
    }

    // calculate the log2 num lanes
    args.log2_num_lanes = Log2(args.num_lanes);

    args.bootstrap = (!bootstrap_address.empty());
    if (args.bootstrap && args.token.size())
    {
      // create the boostrap node
      bootstrap = std::make_unique<fetch::BootstrapMonitor>(
          prover.identity(), args.port, args.network_id, args.token, args.host_name);

      // augment the peer list with the bootstrapped version
      if (bootstrap->Start(args.peers))
      {
        args.interface = bootstrap->interface_address();

        if (args.external_address.empty())
        {
          args.external_address = bootstrap->external_address();
        }
      }
    }

    if (args.external_address.empty())
    {
      args.external_address = "127.0.0.1";
    }

    return args;
  }

  void SetPeers(std::string const &raw_peers)
  {
    if (!raw_peers.empty())
    {
      // split the peers
      std::size_t         position = 0;
      fetch::network::Uri peer;
      while (std::string::npos != position)
      {
        // locate the separator
        std::size_t const separator_position = raw_peers.find(',', position);

        // parse the peer
        std::string const peer_address =
            raw_peers.substr(position, (separator_position - position));
        if (peer.Parse(peer_address))
        {
          peers.push_back(peer);
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse input peer address: '", peer_address, "'");
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
    s << '\n';
    s << "port...........: " << args.port << '\n';
    s << "network id.....: 0x" << std::hex << args.network_id << std::dec << '\n';
    s << "num executors..: " << args.num_executors << '\n';
    s << "num lanes......: " << args.num_lanes << '\n';
    s << "num slices.....: " << args.num_slices << '\n';
    s << "bootstrap......: " << args.bootstrap << '\n';
    s << "host name......: " << args.host_name << '\n';
    s << "external addr..: " << args.external_address << '\n';
    s << "db-prefix......: " << args.dbdir << '\n';
    s << "interface......: " << args.interface << '\n';
    s << "mining.........: " << args.mine << '\n';

    // generate the peer listing
    s << "peers..........: ";
    for (auto const &peer : args.peers)
    {
      s << peer.uri() << ' ';
    }

    // terminate and flush
    s << std::endl;

    return s;
  }
};

ProverPtr GenerateP2PKey()
{
  static constexpr char const *KEY_FILENAME = "p2p.key";

  using Signer    = fetch::crypto::ECDSASigner;
  using SignerPtr = std::unique_ptr<Signer>;

  SignerPtr certificate        = std::make_unique<Signer>();
  bool      certificate_loaded = false;

  // Step 1. Attempt to load the existing key
  {
    std::ifstream input_file(KEY_FILENAME, std::ios::in | std::ios::binary);

    if (input_file.is_open())
    {
      fetch::byte_array::ByteArray private_key_data;
      private_key_data.Resize(Signer::PrivateKey::ecdsa_curve_type::privateKeySize);

      // attempt to read in the private key
      input_file.read(private_key_data.char_pointer(),
                      static_cast<std::streamsize>(private_key_data.size()));

      if (!(input_file.fail() || input_file.eof()))
      {
        certificate->Load(private_key_data);
        certificate_loaded = true;
      }
    }
  }

  // Generate a key if the load failed
  if (!certificate_loaded)
  {
    certificate->GenerateKeys();

    std::ofstream output_file(KEY_FILENAME, std::ios::out | std::ios::binary);

    if (output_file.is_open())
    {
      auto const private_key_data = certificate->private_key();

      output_file.write(private_key_data.char_pointer(),
                        static_cast<std::streamsize>(private_key_data.size()));
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to save P2P key");
    }
  }

  return certificate;
}

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

  if (gConstellationInstance)
  {
    gConstellationInstance.load()->SignalStop();
  }

  if (interrupt_count >= 3)
  {
    std::exit(1);
  }
}

}  // namespace

int main(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;

  fetch::commandline::DisplayCLIHeader("Constellation");

  if (!fetch::version::VALID)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unsupported version");
  }

  try
  {
#ifdef FETCH_ENABLE_METRICS
    fetch::ledger::Metrics::Instance().ConfigureFileHandler("metrics.csv");
#endif  // FETCH_ENABLE_METRICS

    // create and load the main certificate for the bootstrapper
    ProverPtr p2p_key = GenerateP2PKey();

    BootstrapPtr bootstrap_monitor;
    auto const   args = CommandLineArguments::Parse(argc, argv, bootstrap_monitor, *p2p_key);

    FETCH_LOG_INFO(LOGGING_NAME, "Configuration:\n", args);

    // create and run the constellation
    auto constellation = std::make_unique<fetch::Constellation>(
        std::move(p2p_key), args.port, args.num_executors, args.log2_num_lanes, args.num_slices,
        args.interface, args.dbdir, args.external_address);

    // update the instance pointer
    gConstellationInstance = constellation.get();

    // register the signal handler
    std::signal(SIGINT, InterruptHandler);

    // run the application
    constellation->Run(args.peers, args.mine);

    // stop the bootstrapper if we have one
    if (bootstrap_monitor)
    {
      bootstrap_monitor->Stop();
    }

    exit_code = EXIT_SUCCESS;
  }
  catch (std::exception &ex)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Fatal Error: ", ex.what());
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  }

  return exit_code;
}
