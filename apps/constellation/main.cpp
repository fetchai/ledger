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

#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/commandline/params.hpp"
#include "core/json/document.hpp"
#include "core/macros.hpp"
#include "core/string/to_lower.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/fetch_identity.hpp"
#include "crypto/prover.hpp"
#include "metrics/metrics.hpp"
#include "network/adapters.hpp"
#include "network/fetch_asio.hpp"
#include "network/management/network_manager.hpp"
#include "variant/variant.hpp"

#include "bootstrap_monitor.hpp"
#include "constellation.hpp"
#include "fetch_version.hpp"

#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {

constexpr char const *LOGGING_NAME = "main";

using Prover         = fetch::crypto::Prover;
using BootstrapPtr   = std::unique_ptr<fetch::BootstrapMonitor>;
using ProverPtr      = std::shared_ptr<Prover>;
using ConstByteArray = fetch::byte_array::ConstByteArray;
using ByteArray      = fetch::byte_array::ByteArray;
using NetworkMode    = fetch::Constellation::NetworkMode;

std::unordered_set<std::string> const BOOL_TRUE_STRINGS{"true", "enabled", "on", "yes", "1"};
std::unordered_set<std::string> const BOOL_FALSE_STRINGS{"false", "disabled", "off", "no", "0"};

std::atomic<fetch::Constellation *> gConstellationInstance{nullptr};
std::atomic<std::size_t>            gInterruptCount{0};

char const *ToString(NetworkMode network_mode)
{
  char const *text = "Unknown";

  switch (network_mode)
  {
  case NetworkMode::STANDALONE:
    text = "Standalone";
    break;
  case NetworkMode::PRIVATE_NETWORK:
    text = "Private";
    break;
  case NetworkMode::PUBLIC_NETWORK:
    text = "Public";
    break;
  }

  return text;
}

char const *ToYesNo(bool value)
{
  return (value) ? "Yes" : "No";
}

template <typename Value, typename Container>
bool IsIn(Container const &container, Value const &value)
{
  return container.find(value) != container.end();
}

template <typename T>
void UpdateConfigFromEnvironment(T &value, char const *name)
{
  char *environment_value = std::getenv(name);

  if (environment_value != nullptr)
  {
    std::istringstream oss{std::string(environment_value)};
    oss >> value;
  }
}

void UpdateConfigFromEnvironment(bool &value, char const *name)
{
  char *environment_value = std::getenv(name);

  if (environment_value != nullptr)
  {
    std::string string_value{environment_value};
    fetch::string::ToLower(string_value);

    if (IsIn(BOOL_TRUE_STRINGS, string_value))
    {
      value = true;
    }
    else if (IsIn(BOOL_FALSE_STRINGS, string_value))
    {
      value = false;
    }
  }
}

// REVIEW: Move to platform
uint32_t Log2(uint32_t value)
{
  static constexpr uint32_t VALUE_SIZE_IN_BITS = sizeof(value) << 3u;
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

struct CommandLineArguments
{
  using StringList  = std::vector<std::string>;
  using UriList     = fetch::Constellation::UriList;
  using AdapterList = fetch::network::Adapter::adapter_list_type;
  using Manifest    = fetch::network::Manifest;
  using ManifestPtr = std::unique_ptr<Manifest>;
  using Uri         = fetch::network::Uri;
  using Peer        = fetch::network::Peer;
  using Config      = fetch::Constellation::Config;
  using Params      = fetch::commandline::Params;

  using ServiceIdentifier = fetch::network::ServiceIdentifier;
  using ServiceType       = fetch::network::ServiceType;

  static constexpr uint16_t HTTP_PORT_OFFSET    = 0;
  static constexpr uint16_t P2P_PORT_OFFSET     = 1;
  static constexpr uint16_t STORAGE_PORT_OFFSET = 10;

  static const uint32_t DEFAULT_NUM_LANES       = 1;
  static const uint32_t DEFAULT_NUM_SLICES      = 500;
  static const uint32_t DEFAULT_NUM_EXECUTORS   = DEFAULT_NUM_LANES;
  static const uint16_t DEFAULT_PORT            = 8000;
  static const uint32_t DEFAULT_BLOCK_INTERVAL  = 0;  // milliseconds - zero means no mining
  static const uint32_t DEFAULT_MAX_PEERS       = 3;
  static const uint32_t DEFAULT_TRANSIENT_PEERS = 1;

  /// @name Constellation Config
  /// @{
  Config      cfg;
  uint16_t    port{0};
  std::string network_name;
  UriList     peers;
  /// @}

  /// @name Bootstrap Config
  /// @{
  bool        bootstrap{false};
  bool        discoverable{false};
  std::string host_name;
  std::string token;
  std::string external_address{};
  /// @}

  static CommandLineArguments Parse(int argc, char **argv, BootstrapPtr &bootstrap,
                                    ProverPtr const &prover)
  {
    CommandLineArguments args;

    // define the parameters
    Params      p;
    std::string bootstrap_address;
    std::string config_path;
    std::string raw_peers;
    ManifestPtr manifest;
    uint32_t    num_lanes{0};

    bool standalone_flag{false};
    bool private_flag{false};

    // clang-format off
    p.add(args.port,                      "port",                  "The starting port for ledger services",                                                        DEFAULT_PORT);
    p.add(args.cfg.num_executors,         "executors",             "The number of executors to configure",                                                         DEFAULT_NUM_EXECUTORS);
    p.add(num_lanes,                      "lanes",                 "The number of lanes to be used",                                                               DEFAULT_NUM_LANES);
    p.add(args.cfg.num_slices,            "slices",                "The number of slices to be used",                                                              DEFAULT_NUM_SLICES);
    p.add(raw_peers,                      "peers",                 "The comma separated list of addresses to initially connect to",                                std::string{});
    p.add(args.cfg.db_prefix,             "db-prefix",             "The directory or prefix added to the node storage",                                            std::string{"node_storage"});
    p.add(args.network_name,              "network",               "The network name to join",                                                                     std::string{});
    p.add(args.cfg.interface_address,     "interface",             "The address of the network interface to be used",                                              std::string{"127.0.0.1"});
    p.add(args.cfg.block_interval_ms,     "block-interval",        "Block interval in milliseconds",                                                               uint32_t{DEFAULT_BLOCK_INTERVAL});
    p.add(args.token,                     "token",                 "The authentication token to be used with bootstrapping the client",                            std::string{});
    p.add(args.external_address,          "external",              "This node's global IP address",                                                                std::string{});
    p.add(bootstrap_address,              "bootstrap",             "Src address for network bootstrap",                                                            std::string{});
    p.add(args.discoverable,              "discoverable",          "Signal that the client is willing to be listed on the bootstrap server",                       false);
    p.add(args.host_name,                 "host-name",             "The hostname / identifier for this node",                                                      std::string{});
    p.add(config_path,                    "config",                "The path to the manifest configuration",                                                       std::string{});
    p.add(args.cfg.processor_threads,     "processor-threads",     "The number of processor threads",                                                              uint32_t{std::thread::hardware_concurrency()});
    p.add(args.cfg.verification_threads,  "verifier-threads",      "The number of processor threads",                                                              uint32_t{std::thread::hardware_concurrency()});
    p.add(args.cfg.max_peers,             "max-peers",             "The number of maximal peers to send to peer requests",                                         DEFAULT_MAX_PEERS);
    p.add(args.cfg.transient_peers,       "transient-peers",       "The number of the peers which will be random in answer sent to peer requests",                 DEFAULT_TRANSIENT_PEERS);
    p.add(args.cfg.peers_update_cycle_ms, "peers-update-cycle-ms", "How fast to do peering changes",                                                               uint32_t{0});
    p.add(args.cfg.disable_signing,       "disable-signing",       "Do not sign outbound packets or verify those inbound, in trusted network",                     false);
    p.add(args.cfg.sign_broadcasts,       "sign-broadcasts",       "Sign and verify broadcast packets",                                                            false);
    p.add(standalone_flag,                "standalone",            "Run node on its own (useful for testing and development). Incompatible with -private-network", false);
    p.add(private_flag,                   "private-network",       "Run node as part of a private network (disables bootstrap). Incompatible with -standalone",    false);
    // clang-format on

    // parse the args
    p.Parse(argc, argv);

    // if the user has specified any environment variables these always take precedence
    // clang-format off
    UpdateConfigFromEnvironment(args.port,                      "CONSTELLATION_PORT");
    UpdateConfigFromEnvironment(args.cfg.num_executors,         "CONSTELLATION_EXECUTORS");
    UpdateConfigFromEnvironment(num_lanes,                      "CONSTELLATION_LANES");
    UpdateConfigFromEnvironment(args.cfg.num_slices,            "CONSTELLATION_SLICES");
    UpdateConfigFromEnvironment(raw_peers,                      "CONSTELLATION_PEERS");
    UpdateConfigFromEnvironment(args.cfg.db_prefix,             "CONSTELLATION_DB_PREFIX");
    UpdateConfigFromEnvironment(args.network_name,              "CONSTELLATION_NETWORK");
    UpdateConfigFromEnvironment(args.cfg.interface_address,     "CONSTELLATION_INTERFACE");
    UpdateConfigFromEnvironment(args.cfg.block_interval_ms,     "CONSTELLATION_BLOCK_INTERVAL");
    UpdateConfigFromEnvironment(args.token,                     "CONSTELLATION_TOKEN");
    UpdateConfigFromEnvironment(args.external_address,          "CONSTELLATION_EXTERNAL");
    UpdateConfigFromEnvironment(bootstrap_address,              "CONSTELLATION_BOOTSTRAP");
    UpdateConfigFromEnvironment(args.discoverable,              "CONSTELLATION_DISCOVERABLE");
    UpdateConfigFromEnvironment(args.host_name,                 "CONSTELLATION_HOST_NAME");
    UpdateConfigFromEnvironment(config_path,                    "CONSTELLATION_CONFIG");
    UpdateConfigFromEnvironment(args.cfg.processor_threads,     "CONSTELLATION_PROCESSOR_THREADS");
    UpdateConfigFromEnvironment(args.cfg.verification_threads,  "CONSTELLATION_VERIFIER_THREADS");
    UpdateConfigFromEnvironment(args.cfg.max_peers,             "CONSTELLATION_MAX_PEERS");
    UpdateConfigFromEnvironment(args.cfg.transient_peers,       "CONSTELLATION_TRANSIENT_PEERS");
    UpdateConfigFromEnvironment(args.cfg.peers_update_cycle_ms, "CONSTELLATION_PEERS_UPDATE_CYCLE_MS");
    UpdateConfigFromEnvironment(args.cfg.disable_signing,       "CONSTELLATION_DISABLE_SIGNING");
    UpdateConfigFromEnvironment(args.cfg.sign_broadcasts,       "CONSTELLATION_SIGN_BROADCASTS");
    UpdateConfigFromEnvironment(standalone_flag,                "CONSTELLATION_STANDALONE");
    UpdateConfigFromEnvironment(private_flag,                   "CONSTELLATION_PRIVATE_NETWORK");
    // clang-format on

    // update the peers
    args.SetPeers(raw_peers);

    // ensure that the number lanes is a valid power of 2
    if (!EnsureLog2(num_lanes))
    {
      std::cout << "Number of lanes is not a valid log2 number" << std::endl;
      std::exit(1);
    }

    // calculate the log2 num lanes
    args.cfg.log2_num_lanes = Log2(num_lanes);

    // if the user has explicitly passed a configuration then we must parse it here
    bool const manifest_found = LoadManifestFromEnvironment(manifest);

    // if we have not already found a manifest then use the command line version
    if (!manifest_found && !config_path.empty())
    {
      // read the contents of the manifest from the path specified
      manifest = LoadManifestFromFile(config_path.c_str());
    }

    // configure the network mode
    if (standalone_flag && private_flag)
    {
      std::cout
          << "Invalid configuration, unable to be in both the standalone and private network mode"
          << std::endl;
      std::exit(1);
    }
    else if (standalone_flag)
    {
      args.cfg.network_mode = NetworkMode::STANDALONE;
    }
    else if (private_flag)
    {
      args.cfg.network_mode = NetworkMode::PRIVATE_NETWORK;
    }

    args.bootstrap = (!bootstrap_address.empty());
    if (args.bootstrap)
    {
      // bootstrapping is not allowed in a non-public network
      if (NetworkMode::PUBLIC_NETWORK != args.cfg.network_mode)
      {
        std::cout << "Unable to use bootstrapping servers with a private or standalone network"
                  << std::endl;
        std::exit(1);
      }

      // determine what the P2P port is. This is either specified with the port parameter or
      // explicitly given via the manifest
      auto p2p_port = static_cast<uint16_t>(args.port + P2P_PORT_OFFSET);

      // if we have a valid manifest then we should respect the port configuration specified here
      // otherwise we default to the port specified
      if (manifest)
      {
        auto const &uri = manifest->GetUri(ServiceIdentifier{ServiceType::CORE});

        if (uri.scheme() == Uri::Scheme::Tcp)
        {
          p2p_port = uri.AsPeer().port();
        }
      }

      // create the bootstrap node
      bootstrap = std::make_unique<fetch::BootstrapMonitor>(
          prover, p2p_port, args.network_name, args.discoverable, args.token, args.host_name);

      // augment the peer list with the bootstrapped version
      if (bootstrap->DiscoverPeers(args.peers, args.external_address))
      {
        args.external_address = args.cfg.interface_address = bootstrap->external_address();
      }
      else
      {
        // if we have enabled bootstrap and the process fails we must simply exit
        std::exit(1);
      }
    }

    if (args.external_address.empty())
    {
      args.external_address = "127.0.0.1";
    }

    // if we do not have a correct
    if (!manifest)
    {
      manifest = GenerateManifest(args.external_address, args.port, num_lanes);
    }

    // catch the invalid miner case
    if (args.cfg.block_interval_ms > 0)
    {
      auto const identity = prover->identity();

      bool const is_public_network     = NetworkMode::PUBLIC_NETWORK == args.cfg.network_mode;
      bool const is_non_fetch_identity = !fetch::crypto::IsFetchIdentity(identity);

      if (is_public_network && is_non_fetch_identity)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Non mining address: ", identity.identifier().ToBase64());
        std::exit(1);
      }
    }

    // finally
    args.cfg.manifest = std::move(*manifest);

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
    manifest->AddService(ServiceIdentifier{ServiceType::CORE}, Manifest::Entry{Uri{peer}});

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

  static bool LoadManifestFromEnvironment(ManifestPtr &manifest)
  {
    bool present{false};

    char const *manifest_data = std::getenv("CONSTELLATION_MANIFEST");
    if (manifest_data != nullptr)
    {
      // decode the manifest data
      ConstByteArray config = fetch::byte_array::FromBase64(manifest_data);

      ManifestPtr local_manifest = std::make_unique<Manifest>();
      if (!local_manifest->Parse(config))
      {
        throw std::runtime_error("Unable to parse the contents of the manifest file");
      }

      manifest = std::move(local_manifest);
      present  = true;
    }

    return present;
  }

  friend std::ostream &operator<<(std::ostream &              s,
                                  CommandLineArguments const &args) FETCH_MAYBE_UNUSED
  {
    s << '\n';
    s << "port......................: " << args.port << '\n';
    s << "network mode..............: " << ToString(args.cfg.network_mode) << '\n';

    // only print network name when it contains something
    if (!args.network_name.empty())
    {
      s << "network name..............: " << args.network_name << '\n';
    }

    s << "num executors.............: " << args.cfg.num_executors << '\n';
    s << "num lanes.................: " << (1u << args.cfg.log2_num_lanes) << '\n';
    s << "num slices................: " << args.cfg.num_slices << '\n';
    s << "bootstrap.................: " << args.bootstrap << '\n';
    s << "discoverable..............: " << args.discoverable << '\n';
    s << "host name.................: " << args.host_name << '\n';
    s << "external address..........: " << args.external_address << '\n';
    s << "db-prefix.................: " << args.cfg.db_prefix << '\n';
    s << "interface.................: " << args.cfg.interface_address << '\n';
    s << "mining....................: " << ToYesNo(args.cfg.block_interval_ms > 0) << '\n';
    s << "tx processor threads......: " << args.cfg.processor_threads << '\n';
    s << "shard verification threads: " << args.cfg.verification_threads << '\n';
    s << "block interval............: " << args.cfg.block_interval_ms << "ms" << '\n';
    s << "max peers.................: " << args.cfg.max_peers << '\n';
    s << "peers update cycle........: " << args.cfg.peers_update_cycle_ms << "ms\n";

    // generate the peer listing
    s << "peers.....................: ";
    for (auto const &peer : args.peers)
    {
      s << peer.uri() << ' ';
    }

    s << '\n' << "manifest.......: " << args.cfg.manifest.ToString() << '\n';

    // terminate and flush
    s << std::endl;

    return s;
  }
};

ProverPtr GenerateP2PKey()
{
  static constexpr char const *KEY_FILENAME = "p2p.key";

  std::string key_path{KEY_FILENAME};
  UpdateConfigFromEnvironment(key_path, "CONSTELLATION_KEY_PATH");

  using Signer    = fetch::crypto::ECDSASigner;
  using SignerPtr = std::shared_ptr<Signer>;

  SignerPtr certificate        = std::make_shared<Signer>();
  bool      certificate_loaded = false;

  // Step 1. Attempt to load the existing key
  {
    std::ifstream input_file(key_path.c_str(), std::ios::in | std::ios::binary);

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

}  // namespace

int main(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;

  // Special case for the version flag
  if (HasVersionFlag(argc, argv))
  {
    std::cout << fetch::version::FULL << std::endl;
    return 0;
  }

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
    auto         args = CommandLineArguments::Parse(argc, argv, bootstrap_monitor, p2p_key);

    FETCH_LOG_INFO(LOGGING_NAME, "Configuration:\n", args);

    // create and run the constellation
    auto constellation = std::make_unique<fetch::Constellation>(std::move(p2p_key), args.cfg);

    // update the instance pointer
    gConstellationInstance = constellation.get();

    // register the signal handlers
    std::signal(SIGINT, InterruptHandler);
    std::signal(SIGTERM, InterruptHandler);

    // extract the state machine runnable (if any)
    fetch::core::WeakRunnable bootstrap_state_machine{};
    if (bootstrap_monitor)
    {
      bootstrap_state_machine = bootstrap_monitor->GetWeakRunnable();
    }

    // run the application
    constellation->Run(args.peers, bootstrap_state_machine);

    exit_code = EXIT_SUCCESS;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Fatal Error: ", ex.what());
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  }

  return exit_code;
}
