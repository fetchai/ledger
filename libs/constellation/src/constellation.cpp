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

#include "beacon/beacon_service.hpp"
#include "beacon/beacon_setup_service.hpp"
#include "beacon/event_manager.hpp"
#include "bloom_filter/bloom_filter.hpp"
#include "constellation/constellation.hpp"
#include "constellation/health_check_http_module.hpp"
#include "constellation/logging_http_module.hpp"
#include "constellation/muddle_status_http_module.hpp"
#include "constellation/open_api_http_module.hpp"
#include "constellation/telemetry_http_module.hpp"
#include "http/middleware/allow_origin.hpp"
#include "http/middleware/telemetry.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/contract_http_interface.hpp"
#include "ledger/consensus/consensus.hpp"
#include "ledger/consensus/simulated_pow_consensus.hpp"
#include "ledger/consensus/stake_snapshot.hpp"
#include "ledger/dag/dag_interface.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/storage_unit/lane_remote_control.hpp"
#include "ledger/tx_query_http_interface.hpp"
#include "ledger/tx_status_http_interface.hpp"
#include "ledger/upow/synergetic_execution_manager.hpp"
#include "ledger/upow/synergetic_executor.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "network/generics/atomic_inflight_counter.hpp"
#include "network/p2pservice/p2p_http_interface.hpp"
#include "network/uri.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/registry.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>

using fetch::byte_array::ToBase64;
using fetch::chain::Address;
using fetch::ledger::Executor;
using fetch::ledger::GenesisFileCreator;
using fetch::shards::Manifest;
using fetch::shards::ServiceIdentifier;
using fetch::muddle::MuddleInterface;
using fetch::network::AtomicCounterName;
using fetch::network::AtomicInFlightCounter;
using fetch::network::NetworkManager;
using fetch::ledger::StorageInterface;

using ExecutorPtr = std::shared_ptr<Executor>;

namespace fetch {
namespace constellation {

using BeaconSetupServicePtr = std::shared_ptr<beacon::BeaconSetupService>;
using BeaconServicePtr      = std::shared_ptr<fetch::beacon::BeaconService>;
using CertificatePtr        = constellation::Constellation::CertificatePtr;
using Config                = constellation::Constellation::Config;
using ConsensusPtr          = constellation::Constellation::ConsensusPtr;
using ConstByteArray        = byte_array::ConstByteArray;
using EntropyPtr            = std::unique_ptr<ledger::EntropyGeneratorInterface>;
using Identity              = crypto::Identity;
using LaneIndex             = uint32_t;
using MainChain             = ledger::MainChain;
using StakeManagerPtr       = std::shared_ptr<ledger::StakeManager>;

constexpr char const *LOGGING_NAME = "constellation";

const std::size_t HTTP_THREADS{4};
char const *      GENESIS_FILENAME = "genesis_file.json";

bool WaitForLaneServersToStart()
{
  using InFlightCounter = AtomicInFlightCounter<AtomicCounterName::TCP_PORT_STARTUP>;

  core::FutureTimepoint const deadline(std::chrono::seconds(30));

  return InFlightCounter::Wait(deadline);
}

std::size_t CalcNetworkManagerThreads(std::size_t num_lanes)
{
  static constexpr std::size_t THREADS_PER_LANE = 4;
  static constexpr std::size_t OTHER_THREADS    = 10;

  return (num_lanes * THREADS_PER_LANE) + OTHER_THREADS;
}

uint16_t LookupRemotePort(Manifest const &manifest, ServiceIdentifier::Type service,
                          uint32_t instance = ServiceIdentifier::SINGLETON_SERVICE)
{
  ServiceIdentifier const identifier{service, instance};

  auto it = manifest.FindService(identifier);
  if (it == manifest.end())
  {
    throw std::runtime_error("Unable to lookup requested service from the manifest");
  }

  return it->second.uri().GetTcpPeer().port();
}

uint16_t LookupLocalPort(Manifest const &manifest, ServiceIdentifier::Type service,
                         uint32_t instance = ServiceIdentifier::SINGLETON_SERVICE)
{
  ServiceIdentifier const identifier{service, instance};

  auto it = manifest.FindService(identifier);
  if (it == manifest.end())
  {
    throw std::runtime_error("Unable to look up requested service from the manifest");
  }

  return it->second.local_port();
}

std::shared_ptr<ledger::DAGInterface> GenerateDAG(
    Config const &cfg, std::string const &db_name, bool load_on_start,
    constellation::Constellation::CertificatePtr certificate)
{
  if (cfg.features.IsEnabled("synergetic"))
  {
    return std::make_shared<ledger::DAG>(db_name, load_on_start, certificate);
  }
  return {};
}

ledger::ShardConfigs GenerateShardsConfig(Config &cfg, uint16_t start_port)
{
  ledger::ShardConfigs configs(cfg.num_lanes());

  for (uint32_t i = 0; i < cfg.num_lanes(); ++i)
  {
    // look up the service in the provided manifest
    auto it = cfg.manifest.FindService(ServiceIdentifier{ServiceIdentifier::Type::LANE, i});

    if (it == cfg.manifest.end())
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Unable to update manifest for lane ", i);
      throw std::runtime_error("Invalid manifest provided");
    }

    auto &shard = configs[i];

    shard.lane_id           = i;
    shard.num_lanes         = cfg.num_lanes();
    shard.storage_path      = cfg.db_prefix;
    shard.external_name     = it->second.uri().GetTcpPeer().address();
    shard.external_identity = std::make_shared<crypto::ECDSASigner>();
    shard.external_port     = start_port++;
    shard.external_network_id =
        muddle::NetworkId{(static_cast<uint32_t>(i) & 0xFFFFFFu) | (uint32_t{'L'} << 24u)};
    shard.internal_name        = it->second.uri().GetTcpPeer().address();
    shard.internal_identity    = std::make_shared<crypto::ECDSASigner>();
    shard.internal_port        = start_port++;
    shard.internal_network_id  = muddle::NetworkId{"ISRD"};
    shard.verification_threads = cfg.verification_threads;

    auto const ext_identity = shard.external_identity->identity().identifier();
    auto const int_identity = shard.internal_identity->identity().identifier();

    FETCH_LOG_INFO(LOGGING_NAME, "Shard ", i + 1);
    FETCH_LOG_INFO(LOGGING_NAME, " - Internal ", ToBase64(int_identity), " - ",
                   shard.internal_network_id.ToString(), " - tcp://0.0.0.0:", shard.internal_port);
    FETCH_LOG_INFO(LOGGING_NAME, " - External ", ToBase64(ext_identity), " - ",
                   shard.external_network_id.ToString(), " - tcp://0.0.0.0:", shard.external_port);

    // update the manifest with the generated identity
    it->second.UpdateAddress(ext_identity);
  }

  return configs;
}

StakeManagerPtr CreateStakeManager(constellation::Constellation::Config const &cfg)
{
  StakeManagerPtr mgr{};

  if (cfg.proof_of_stake)
  {
    mgr = std::make_shared<ledger::StakeManager>();
  }

  return mgr;
}

ConsensusPtr CreateConsensus(constellation::Constellation::Config const &cfg, StakeManagerPtr stake,
                             BeaconSetupServicePtr beacon_setup, BeaconServicePtr beacon,
                             MainChain const &chain, StorageInterface &storage,
                             Identity const &identity)
{
  ConsensusPtr consensus{};

  if (stake)
  {
    consensus = std::make_shared<ledger::Consensus>(stake, beacon_setup, beacon, chain, storage,
                                                    identity, cfg.aeon_period, cfg.max_cabinet_size,
                                                    cfg.block_interval_ms);
  }
  else
  {
    consensus = std::make_shared<ledger::SimulatedPowConsensus>(identity, cfg.block_interval_ms);
  }

  return consensus;
}

muddle::MuddlePtr CreateBeaconNetwork(Config const &cfg, CertificatePtr certificate,
                                      NetworkManager const &nm)
{
  muddle::MuddlePtr network;

  if (cfg.proof_of_stake)
  {
    network = muddle::CreateMuddle("DKGN", std::move(certificate), nm,
                                   cfg.manifest.FindExternalAddress(ServiceIdentifier::Type::DKG));
  }

  return network;
}

BeaconSetupServicePtr CreateBeaconSetupService(constellation::Constellation::Config const &cfg,
                                               MuddleInterface &                           muddle,
                                               shards::ShardManagementService &manifest_cache,
                                               CertificatePtr                  certificate)
{
  BeaconSetupServicePtr beacon_setup{};
  if (cfg.proof_of_stake)
  {
    beacon_setup =
        std::make_unique<fetch::beacon::BeaconSetupService>(muddle, manifest_cache, certificate);
  }
  return beacon_setup;
}

BeaconServicePtr CreateBeaconService(constellation::Constellation::Config const &cfg,
                                     MuddleInterface &muddle, CertificatePtr certificate,
                                     BeaconSetupServicePtr const &beacon_setup)
{
  BeaconServicePtr                         beacon{};
  beacon::EventManager::SharedEventManager event_manager = beacon::EventManager::New();

  if (cfg.proof_of_stake)
  {
    assert(beacon_setup);
    beacon = std::make_unique<fetch::beacon::BeaconService>(muddle, certificate, *beacon_setup,
                                                            event_manager, true);
  }

  return beacon;
}

/**
 * Construct a constellation instance
 *
 * @param certificate The reference to the node public key
 * @param manifest The service manifest for this instance
 * @param port_start The start port for all the services
 * @param num_executors The number of executors
 * @param num_lanes The configured number of lanes
 * @param num_slices The configured number of slices
 * @param interface_address The current interface address TODO(EJF): This should be more integrated
 * @param db_prefix The database file(s) prefix
 */
Constellation::Constellation(CertificatePtr const &certificate, Config config)
  : active_{true}
  , cfg_{std::move(config)}
  , p2p_port_(LookupLocalPort(cfg_.manifest, ServiceIdentifier::Type::CORE))
  , http_port_(LookupLocalPort(cfg_.manifest, ServiceIdentifier::Type::HTTP))
  , lane_port_start_(LookupLocalPort(cfg_.manifest, ServiceIdentifier::Type::LANE, 0))
  , shard_cfgs_{GenerateShardsConfig(cfg_, lane_port_start_)}
  , reactor_{"Reactor"}
  , network_manager_{"NetMgr", CalcNetworkManagerThreads(cfg_.num_lanes())}
  , http_network_manager_{"Http", HTTP_THREADS}
  , muddle_{muddle::CreateMuddle("IHUB", certificate, network_manager_,
                                 cfg_.manifest.FindExternalAddress(ServiceIdentifier::Type::CORE))}
  , internal_identity_{std::make_shared<crypto::ECDSASigner>()}
  , external_identity_{certificate}
  , internal_muddle_{muddle::CreateMuddle(
        "ISRD", internal_identity_, network_manager_,
        cfg_.manifest.FindExternalAddress(ServiceIdentifier::Type::CORE))}
  , tx_status_cache_(TxStatusCache::factory())
  , storage_(std::make_shared<StorageUnitClient>(internal_muddle_->GetEndpoint(), shard_cfgs_,
                                                 cfg_.log2_num_lanes))
  , lane_control_(internal_muddle_->GetEndpoint(), shard_cfgs_, cfg_.log2_num_lanes)
  , shard_management_(std::make_shared<ShardManagementService>(cfg_.manifest, lane_control_,
                                                               *muddle_, cfg_.log2_num_lanes))
  , dag_{GenerateDAG(cfg_, "dag_db_", true, certificate)}
  , beacon_network_{CreateBeaconNetwork(cfg_, certificate, network_manager_)}
  , beacon_setup_{CreateBeaconSetupService(cfg_, *beacon_network_, *shard_management_, certificate)}
  , beacon_{CreateBeaconService(cfg_, *beacon_network_, certificate, beacon_setup_)}
  , stake_{CreateStakeManager(cfg_)}
  , consensus_{CreateConsensus(cfg_, stake_, beacon_setup_, beacon_, chain_, *storage_,
                               certificate->identity())}
  , execution_manager_{std::make_shared<ExecutionManager>(
        cfg_.num_executors, cfg_.log2_num_lanes, storage_,
        [this] { return std::make_shared<Executor>(storage_); }, tx_status_cache_)}
  , chain_{cfg_.features.IsEnabled(FeatureFlags::MAIN_CHAIN_BLOOM_FILTER),
           ledger::MainChain::Mode::LOAD_PERSISTENT_DB}
  , block_packer_{cfg_.log2_num_lanes}
  , block_coordinator_{chain_,
                       dag_,
                       *execution_manager_,
                       *storage_,
                       block_packer_,
                       *this,
                       certificate,
                       cfg_.log2_num_lanes,
                       cfg_.num_slices,
                       consensus_,
                       std::make_unique<ledger::SynergeticExecutionManager>(
                           dag_, 1u,
                           [this]() {
                             return std::make_shared<ledger::SynergeticExecutor>(*storage_);
                           })}
  , main_chain_service_{std::make_shared<MainChainRpcService>(muddle_->GetEndpoint(), chain_,
                                                              trust_, cfg_.network_mode)}
  , tx_processor_{dag_, *storage_, block_packer_, tx_status_cache_, cfg_.processor_threads}
  , http_open_api_module_{std::make_shared<OpenAPIHttpModule>()}
  , http_{http_network_manager_}
  , http_modules_{http_open_api_module_,
                  std::make_shared<p2p::P2PHttpInterface>(
                      cfg_.log2_num_lanes, chain_, block_packer_,
                      p2p::P2PHttpInterface::WeakStateMachines{
                          main_chain_service_->GetWeakStateMachine(),
                          block_coordinator_.GetWeakStateMachine()}),
                  std::make_shared<ledger::TxStatusHttpInterface>(tx_status_cache_),
                  std::make_shared<ledger::TxQueryHttpInterface>(*storage_),
                  std::make_shared<ledger::ContractHttpInterface>(*storage_, tx_processor_),
                  std::make_shared<LoggingHttpModule>(),
                  std::make_shared<TelemetryHttpModule>(),
                  std::make_shared<MuddleStatusModule>(),
                  std::make_shared<HealthCheckHttpModule>(chain_, *main_chain_service_,
                                                          block_coordinator_)}
  , uptime_{telemetry::Registry::Instance().CreateCounter(
        "ledger_uptime_ticks_total",
        "The number of intervals that ledger instance has been alive for")}
{
  // print the start up log banner
  FETCH_LOG_INFO(LOGGING_NAME, "Constellation :: ", cfg_.num_lanes(), "x", cfg_.num_slices, "x",
                 cfg_.num_executors);
  FETCH_LOG_INFO(LOGGING_NAME,
                 "              :: ", Address::FromMuddleAddress(muddle_->GetAddress()).display());
  FETCH_LOG_INFO(LOGGING_NAME, "              :: ", muddle_->GetAddress().ToBase64());
  FETCH_LOG_INFO(LOGGING_NAME, "");

  // Configure/override global parameters
  chain::STAKE_WARM_UP_PERIOD   = cfg_.stake_delay_period;
  chain::STAKE_COOL_DOWN_PERIOD = cfg_.stake_delay_period;

  if (cfg_.kademlia_routing)
  {
    muddle_->SetPeerSelectionMode(muddle::PeerSelectionMode::KADEMLIA);
  }

  // Enable experimental features
  if (cfg_.features.IsEnabled("synergetic") && dag_)
  {
    dag_service_ = std::make_shared<ledger::DAGService>(muddle_->GetEndpoint(), dag_);
    reactor_.Attach(dag_service_->GetWeakRunnable());
  }

  if (cfg_.features.IsEnabled("synergetic") && dag_)
  {
    auto syn_miner = std::make_unique<NaiveSynergeticMiner>(dag_, *storage_, certificate);
    if (!reactor_.Attach(syn_miner->GetWeakRunnable()))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to attach synergetic miner to reactor.");
      throw std::runtime_error("Failed to attach synergetic miner to reactor.");
    }
    synergetic_miner_ = std::move(syn_miner);
  }

  // Attach beacon runnables
  if (beacon_)
  {
    reactor_.Attach(beacon_setup_->GetWeakRunnables());
    reactor_.Attach(beacon_->GetWeakRunnable());
  }

  // attach the services to the reactor
  reactor_.Attach(main_chain_service_->GetWeakRunnable());
  reactor_.Attach(shard_management_);

  // configure all the lane services
  lane_services_.Setup(network_manager_, shard_cfgs_);

  // configure the middleware of the http server
  http_.AddMiddleware(http::middleware::AllowOrigin("*"));
  http_.AddMiddleware(http::middleware::Telemetry());

  // attach all the modules to the http server
  for (auto const &module : http_modules_)
  {
    http_.AddModule(*module);
  }
}

/**
 * Writes OpenAPI information about the HTTP REST interface to a stream.
 *
 * @param stream is stream to which the API is dumped to.
 */
void Constellation::DumpOpenAPI(std::ostream &stream)
{
  stream << "paths:" << std::endl;
  byte_array::ConstByteArray last_path{};
  for (auto const &view : http_.views())
  {
    std::string method = ToString(view.method);
    std::transform(method.begin(), method.end(), method.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (last_path != view.route.path())
    {
      stream << "  " << view.route.path() << ":" << std::endl;
    }

    last_path = view.route.path();
    stream << "    " << method << ":" << std::endl;
    stream << "      description: "
           << "\"" << view.description << "\"" << std::endl;
    stream << "      parameters: "
           << "[" << std::endl;
    stream << "      ] " << std::endl;
  }
}

/**
 * Runs the constellation service with the specified initial peers
 *
 * @param initial_peers The peers that should be initially connected to
 */
bool Constellation::Run(UriSet const &initial_peers, core::WeakRunnable bootstrap_monitor)
{
  using Peers = muddle::MuddleInterface::Peers;

  bool startup_success{true};

  //---------------------------------------------------------------
  // Step 1. Start all the components
  //---------------------------------------------------------------

  /// NETWORKING INFRASTRUCTURE

  // start all the services
  http_open_api_module_->Reset(&http_);
  network_manager_.Start();
  http_network_manager_.Start();

  // always use mapping based ports
  muddle::MuddleInterface::PortMapping port_mapping{};
  port_mapping[p2p_port_] = LookupRemotePort(cfg_.manifest, ServiceIdentifier::Type::CORE);

  muddle_->Start(initial_peers, port_mapping);

  /// LANE / SHARD SERVERS

  // start all the lane services and wait for them to start accepting
  // connections
  lane_services_.Start();

  FETCH_LOG_INFO(LOGGING_NAME, "Starting shard services...");
  if (!WaitForLaneServersToStart())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to start lane server instances");
    return false;
  }
  FETCH_LOG_INFO(LOGGING_NAME, "Starting shard services...complete");

  /// LANE / SHARD CLIENTS

  {
    FETCH_LOG_INFO(LOGGING_NAME,
                   "Inter-shard Identity: ", internal_muddle_->GetAddress().ToBase64());

    // build the complete list of Uris to all the lane services across the internal network
    Peers internal_peers{};
    for (auto const &shard : shard_cfgs_)
    {
      internal_peers.emplace("tcp://127.0.0.1:" + std::to_string(shard.internal_port));
    }

    // start the muddle up and connect to all the shards
    internal_muddle_->Start(internal_peers, {});

    // wait for all the connections to establish
    while (active_)
    {
      // exit the wait loop until all the connections have been formed
      if (internal_muddle_->GetNumDirectlyConnectedPeers() >= shard_cfgs_.size())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Internal muddle network established between shards");
        break;
      }

      FETCH_LOG_DEBUG(LOGGING_NAME, "Waiting for internal muddle connection to be established...");

      std::this_thread::sleep_for(std::chrono::milliseconds{500});
    }
  }

  // BEFORE the block coordinator starts its state set up special genesis
  if (cfg_.proof_of_stake || cfg_.load_genesis_file)
  {
    FETCH_LOG_INFO(LOGGING_NAME,
                   "Loading from genesis save file. Location: ", cfg_.genesis_file_location);

    GenesisFileCreator creator(block_coordinator_, *storage_, consensus_, external_identity_,
                               cfg_.db_prefix);

    if (cfg_.genesis_file_location.empty())
    {
      startup_success &= creator.LoadFile(GENESIS_FILENAME);
    }
    else
    {
      startup_success &= creator.LoadFile(cfg_.genesis_file_location);
    }

    if (!startup_success)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to restore state from genesis file");
    }
    else
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Loaded from genesis save file.");
    }
  }

  if (startup_success)
  {
    // beacon network
    if (beacon_network_)
    {
      uint16_t const beacon_bind_port =
          LookupLocalPort(cfg_.manifest, ServiceIdentifier::Type::DKG);
      uint16_t const beacon_ext_port =
          LookupRemotePort(cfg_.manifest, ServiceIdentifier::Type::DKG);

      muddle::MuddleInterface::PortMapping const beacon_port_mapping{
          {beacon_bind_port, beacon_ext_port}};

      beacon_network_->Start({}, beacon_port_mapping);
    }

    // reactor important to run the block/chain state machine
    reactor_.Start();

    /// BLOCK EXECUTION & MINING

    execution_manager_->Start();
    tx_processor_.Start();

    /// INPUT INTERFACES

    // Finally start the HTTP server
    http_.Start(http_port_);

    // The block coordinator needs to access correctly started lanes to recover state in the case of
    // a crash.
    reactor_.Attach(block_coordinator_.GetWeakRunnable());
  }

  //---------------------------------------------------------------
  // Step 2. Main monitor loop
  //---------------------------------------------------------------
  bool start_up_in_progress{true};

  // monitor loop
  while (startup_success && active_)
  {
    // determine the status of the main chain server
    bool const is_in_sync = main_chain_service_->IsSynced() && block_coordinator_.IsSynced();

    if (synergetic_miner_)
    {
      synergetic_miner_->EnableMining(is_in_sync);
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Still alive...");
    std::this_thread::sleep_for(std::chrono::milliseconds{500});

    // detect the first time that we have fully synced
    if (start_up_in_progress && is_in_sync)
    {
      // Attach the bootstrap monitor (if one exists) to the reactor at this point. This starts the
      // monitor state machine. If one doesn't exist (empty weak pointer) then the reactor will
      // simply discard this piece of work.
      //
      // Starting this state machine begins period notify calls to the bootstrap server. This
      // importantly triggers the bootstrap service to start listing this node as available for
      // client connections. By delaying these notify() calls to the point when the node believes
      // it has successfully synchronised this ensures that cleaner network start up
      //
      reactor_.Attach(std::move(bootstrap_monitor));
      start_up_in_progress = false;

      FETCH_LOG_INFO(LOGGING_NAME, "Startup complete");
    }

    // update the uptime counter
    uptime_->increment();
  }

  //---------------------------------------------------------------
  // Step 3. Tear down
  //---------------------------------------------------------------

  FETCH_LOG_INFO(LOGGING_NAME, "Shutting down...");

  http_.Stop();
  tx_processor_.Stop();
  reactor_.Stop();
  execution_manager_->Stop();
  storage_.reset();
  lane_services_.Stop();
  muddle_->Stop();
  internal_muddle_->Stop();
  http_network_manager_.Stop();
  network_manager_.Stop();
  http_open_api_module_->Reset(nullptr);

  FETCH_LOG_INFO(LOGGING_NAME, "Shutting down...complete");

  return startup_success;
}

void Constellation::OnBlock(ledger::Block const &block)
{
  main_chain_service_->BroadcastBlock(block);
}

void Constellation::SignalStop()
{
  active_ = false;
}

}  // namespace constellation
}  // namespace fetch
