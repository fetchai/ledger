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
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "metrics/metrics.hpp"
#include "network/adapters.hpp"
#include "network/fetch_asio.hpp"
#include "network/management/network_manager.hpp"
#include "variant/variant.hpp"

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

using Prover         = fetch::crypto::Prover;
using BootstrapPtr   = std::unique_ptr<fetch::BootstrapMonitor>;
using ProverPtr      = std::unique_ptr<Prover>;
using ConstByteArray = fetch::byte_array::ConstByteArray;
using ByteArray      = fetch::byte_array::ByteArray;

using fetch::chain::consensus::ConsensusMinerType;

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
  return value == (1u << log2_value);
}

ConstByteArray ReadContentsOfFile(char const *filename)
{
  ConstByteArray buffer;

  std::ifstream stream(filename, std::ios::in | std::ios::binary);

  if (stream.is_open())
  {
    // determine the complete size of the file
    stream.seekg(0, std::ios::end);
    std::streamsize const stream_size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    // allocate the buffer
    ByteArray buf;
    buf.Resize(static_cast<std::size_t>(stream_size));
    static_assert(sizeof(stream_size) >= sizeof(std::size_t), "Must be same size or larger");

    // read in size of the buffer
    stream.read(buf.char_pointer(), stream_size);

    // update the output buffer
    buffer = buf;
  }

  return buffer;
}

std::ostream &operator<<(std::ostream &os, const ConsensusMinerType &obj)
{
  os << static_cast<std::underlying_type<ConsensusMinerType>::type>(obj);
  return os;
}

struct CommandLineArguments
{
  using StringList  = std::vector<std::string>;
  using UriList     = fetch::Constellation::UriList;
  using AdapterList = fetch::network::Adapter::adapter_list_type;
  using Manifest    = fetch::network::Manifest;
  using ManifestPtr = std::unique_ptr<Manifest>;
  using Uri         = fetch::network::Uri;
  using Peer        = fetch::network::Peer;

  using ServiceIdentifier = fetch::network::ServiceIdentifier;
  using ServiceType       = fetch::network::ServiceType;

  static constexpr uint16_t HTTP_PORT_OFFSET    = 0;
  static constexpr uint16_t P2P_PORT_OFFSET     = 1;
  static constexpr uint16_t STORAGE_PORT_OFFSET = 10;

  static const uint32_t    DEFAULT_NUM_LANES       = 4;
  static const uint32_t    DEFAULT_NUM_SLICES      = 4;
  static const uint32_t    DEFAULT_NUM_EXECUTORS   = DEFAULT_NUM_LANES;
  static const uint16_t    DEFAULT_PORT            = 8000;
  static const uint32_t    DEFAULT_NETWORK_ID      = 0x10;
  static const uint32_t    DEFAULT_BLOCK_INTERVAL  = 5000;  // milliseconds.
  static const std::size_t DEFAULT_MAX_PEERS       = 3;
  static const std::size_t DEFAULT_TRANSIENT_PEERS = 1;

  uint16_t           port{0};
  uint32_t           network_id;
  UriList            peers;
  uint32_t           num_executors;
  uint32_t           num_lanes;
  uint32_t           log2_num_lanes;
  uint32_t           num_slices;
  uint32_t           block_interval;
  std::string        interface;
  std::string        token;
  bool               bootstrap{false};
  ConsensusMinerType mine{ConsensusMinerType::NO_MINER};
  std::string        dbdir;
  std::string        external_address;
  std::string        host_name;
  ManifestPtr        manifest;
  std::size_t        processor_threads;
  std::size_t        verification_threads;
  std::size_t        max_peers;
  std::size_t        transient_peers;

  static CommandLineArguments Parse(int argc, char **argv, BootstrapPtr &bootstrap,
                                    Prover const &prover)
  {
    CommandLineArguments args;

    // define the parameters
    std::string raw_peers;

    fetch::commandline::Params parameters;
    std::string                bootstrap_address;
    std::string                external_address;
    std::string                config_path;
    int                        mine;

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
    parameters.add(args.block_interval, "block-interval", "Block interval in milliseconds.",
                   uint32_t{DEFAULT_BLOCK_INTERVAL});
    parameters.add(external_address, "bootstrap", "Enable bootstrap network support",
                   std::string{});
    parameters.add(args.token, "token",
                   "The authentication token to be used with bootstrapping the client",
                   std::string{});
    parameters.add(mine, "mine", "Enable mining on this node", 0);

    parameters.add(args.external_address, "external", "This node's global IP addr.", std::string{});
    parameters.add(bootstrap_address, "bootstrap", "Src addr for network boostrap.", std::string{});
    parameters.add(args.host_name, "host-name", "The hostname / identifier for this node",
                   std::string{});
    parameters.add(config_path, "config", "The path to the manifest configuration", std::string{});
    parameters.add(args.processor_threads, "processor-threads", "The number of processor threads",
                   std::size_t{std::thread::hardware_concurrency()});
    parameters.add(args.verification_threads, "verifier-threads", "The number of processor threads",
                   std::size_t{std::thread::hardware_concurrency()});
    parameters.add(args.max_peers, "max-peers",
                   "The number of maximal peers to send to peer requests.", DEFAULT_MAX_PEERS);
    parameters.add(args.transient_peers, "transient-peers",
                   "The number of the peers which will be random in answer sent to peer requests.",
                   DEFAULT_TRANSIENT_PEERS);

    // parse the args
    parameters.Parse(argc, argv);

    // update the peers
    args.SetPeers(raw_peers);

    args.mine = static_cast<ConsensusMinerType>(mine);

    // ensure that the number lanes is a valid power of 2
    if (!EnsureLog2(args.num_lanes))
    {
      std::cout << "Number of lanes is not a valid log2 number" << std::endl;
      std::exit(1);
    }

    // calculate the log2 num lanes
    args.log2_num_lanes = Log2(args.num_lanes);

    // if the user has explicitly passed a configuration then we must parse it here
    if (!config_path.empty())
    {
      // read the contents of the manifest from the path specified
      args.manifest = LoadManifestFromFile(config_path.c_str());
    }

    args.bootstrap = (!bootstrap_address.empty());
    if (args.bootstrap && args.token.size())
    {
      // determine what the P2P port is. This is either specified with the port parameter or
      // explicitly given via the manifest
      uint16_t p2p_port = static_cast<uint16_t>(args.port + P2P_PORT_OFFSET);

      // if we have a valid manifest then we should respect the port configuration specified here
      // otherwise we default to the port specified
      if (args.manifest)
      {
        auto const &uri = args.manifest->GetUri(ServiceIdentifier{ServiceType::P2P});

        if (uri.scheme() == Uri::Scheme::Tcp)
        {
          p2p_port = uri.AsPeer().port();
        }
      }

      // create the bootstrap node
      bootstrap = std::make_unique<fetch::BootstrapMonitor>(
          prover.identity(), p2p_port, args.network_id, args.token, args.host_name);

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

    // if we do not have a correct
    if (!args.manifest)
    {
      args.manifest = GenerateManifest(args.external_address, args.port, args.num_lanes);
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

  static ManifestPtr GenerateManifest(std::string const &external_address, uint16_t port,
                                      uint32_t num_lanes)
  {
    ManifestPtr manifest = std::make_unique<Manifest>();
    Peer        peer;

    // register the HTTP service
    peer.Update(external_address, port + HTTP_PORT_OFFSET);
    manifest->AddService(ServiceIdentifier{ServiceType::HTTP}, Manifest::Entry{Uri{peer}});

    // register the P2P service
    peer.Update(external_address, static_cast<uint16_t>(port + P2P_PORT_OFFSET));
    manifest->AddService(ServiceIdentifier{ServiceType::P2P}, Manifest::Entry{Uri{peer}});

    // register all of the lanes (storage shards)
    for (uint32_t i = 0; i < num_lanes; ++i)
    {
      peer.Update(external_address, static_cast<uint16_t>(port + STORAGE_PORT_OFFSET + i));

      manifest->AddService(ServiceIdentifier{ServiceType::LANE, static_cast<uint16_t>(i)},
                           Manifest::Entry{Uri{peer}});
    }

    return manifest;
  }

  static ManifestPtr LoadManifestFromFile(char const *filename)
  {
    ConstByteArray buffer = ReadContentsOfFile(filename);

    // check to see if the read failed
    if (buffer.size() == 0)
    {
      throw std::runtime_error("Unable to read the contents of the requested file");
    }

    ManifestPtr manifest = std::make_unique<Manifest>();
    if (!manifest->Parse(buffer))
    {
      throw std::runtime_error("Unable to parse the contents of the manifest file");
    }

    return manifest;
  }

  friend std::ostream &operator<<(std::ostream &              s,
                                  CommandLineArguments const &args) FETCH_MAYBE_UNUSED
  {
    s << '\n';
    s << "port......................: " << args.port << '\n';
    s << "network id................: 0x" << std::hex << args.network_id << std::dec << '\n';
    s << "num executors.............: " << args.num_executors << '\n';
    s << "num lanes.................: " << args.num_lanes << '\n';
    s << "num slices................: " << args.num_slices << '\n';
    s << "bootstrap.................: " << args.bootstrap << '\n';
    s << "host name.................: " << args.host_name << '\n';
    s << "external address..........: " << args.external_address << '\n';
    s << "db-prefix.................: " << args.dbdir << '\n';
    s << "interface.................: " << args.interface << '\n';
    s << "mining....................: " << args.mine << '\n';
    s << "tx processor threads......: " << args.processor_threads << '\n';
    s << "shard verification threads: " << args.verification_threads << '\n';
    s << "block interval............: " << args.block_interval << "ms" << std::endl;
    // generate the peer listing
    s << "peers.....................: ";
    for (auto const &peer : args.peers)
    {
      s << peer.uri() << ' ';
    }

    if (args.manifest)
    {
      s << '\n' << "manifest.......: " << args.manifest->ToString() << '\n';
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
    FETCH_LOG_WARN(LOGGING_NAME, "Unsupported version - git working tree is dirty");
  }

  try
  {
#ifdef FETCH_ENABLE_METRICS
    fetch::metrics::Metrics::Instance().ConfigureFileHandler("metrics.csv");
#endif  // FETCH_ENABLE_METRICS

    // create and load the main certificate for the bootstrapper
    ProverPtr p2p_key = GenerateP2PKey();

    BootstrapPtr bootstrap_monitor;
    auto         args = CommandLineArguments::Parse(argc, argv, bootstrap_monitor, *p2p_key);

    FETCH_LOG_INFO(LOGGING_NAME, "Configuration:\n", args);

    // create and run the constellation
    auto constellation = std::make_unique<fetch::Constellation>(
        std::move(p2p_key), std::move(*args.manifest), args.num_executors, args.log2_num_lanes,
        args.num_slices, args.interface, args.dbdir, args.external_address, args.processor_threads,
        args.verification_threads, std::chrono::milliseconds(args.block_interval), args.max_peers,
        args.transient_peers);

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
