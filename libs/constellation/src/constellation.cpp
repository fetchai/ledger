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

#include "constellation/constellation.hpp"

#include "beacon/beacon_service.hpp"
#include "beacon/beacon_setup_service.hpp"
#include "beacon/event_manager.hpp"
#include "bloom_filter/bloom_filter.hpp"
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
#include "ledger/protocols/main_chain_rpc_service.hpp"
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

using fetch::chain::Address;
using fetch::ledger::Executor;
using fetch::ledger::GenesisFileCreator;
using fetch::ledger::StorageInterface;
using fetch::muddle::MuddleInterface;
using fetch::network::AtomicCounterName;
using fetch::network::AtomicInFlightCounter;
using fetch::network::NetworkManager;
using fetch::shards::Manifest;
using fetch::shards::ServiceIdentifier;

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

class Defer
{
public:
  using Callback = void (Constellation::*)();

  Defer(Constellation *instance, Callback callback)
    : instance_{instance}
    , callback_{callback}
  {}

  ~Defer()
  {
    (instance_->*callback_)();
  }

private:
  Constellation *const instance_;
  Callback const       callback_;
};

char const *ToString(ledger::MainChainRpcService::Mode mode)
{
  char const *text = "Unknown";

  switch (mode)
  {
  case ledger::MainChainRpcService::Mode::STANDALONE:
    text = "Standalone";
    break;
  case ledger::MainChainRpcService::Mode::PRIVATE_NETWORK:
    text = "Private";
    break;
  case ledger::MainChainRpcService::Mode::PUBLIC_NETWORK:
    text = "Public";
    break;
  }

  return text;
}

template <typename T>
void ResetItem(T &item)
{
  if (item)
  {
    item.reset();
  }
}

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
    consensus =
        std::make_shared<ledger::SimulatedPowConsensus>(identity, cfg.block_interval_ms, chain);
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

muddle::MuddlePtr CreateMessengerNetwork(Config const &cfg, CertificatePtr const & /*certificate*/,
                                         NetworkManager const & /*nm*/)
{
  muddle::MuddlePtr network;

  if (cfg.enable_agents)
  {
    /*
    TODO: Make work in order to enable
    network =
        muddle::CreateMuddle("AGEN", std::move(certificate), nm,
                             cfg.manifest.FindExternalAddress(ServiceIdentifier::Type::AGENTS));
                             */
  }

  return network;
}

Constellation::MailboxPtr CreateMessengerMailbox(Config const &cfg, muddle::MuddlePtr &network)
{
  Constellation::MailboxPtr ret{nullptr};

  if (cfg.enable_agents && network)
  {
    ret = std::make_unique<Constellation::Mailbox>(network);
  }

  return ret;
}

Constellation::MessengerAPIPtr CreateMessengerAPI(Config const &cfg, muddle::MuddlePtr &network,
                                                  Constellation::MailboxPtr &mailbox)
{
  Constellation::MessengerAPIPtr ret{nullptr};

  if (cfg.enable_agents && network && mailbox)
  {
    ret = std::make_unique<Constellation::MessengerAPI>(network, *mailbox);
  }

  return ret;
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
Constellation::Constellation(CertificatePtr certificate, Config config)
  : active_{true}
  , cfg_{std::move(config)}
  , p2p_port_(LookupLocalPort(cfg_.manifest, ServiceIdentifier::Type::CORE))
  , http_port_(LookupLocalPort(cfg_.manifest, ServiceIdentifier::Type::HTTP))
  , lane_port_start_(LookupLocalPort(cfg_.manifest, ServiceIdentifier::Type::LANE, 0))
  , shard_cfgs_{GenerateShardsConfig(cfg_, lane_port_start_)}
  , reactor_{"Reactor"}
  , reactor_dkg_{"ReactorDKG"}
  , network_manager_{"NetMgr", CalcNetworkManagerThreads(cfg_.num_lanes())}
  , http_network_manager_{"Http", HTTP_THREADS}
  , internal_identity_{std::make_shared<crypto::ECDSASigner>()}
  , external_identity_{std::move(certificate)}
  , tx_status_cache_(TxStatusCache::factory())
  , uptime_{telemetry::Registry::Instance().CreateCounter(
        "ledger_uptime_ticks_total",
        "The number of intervals that ledger instance has been alive for")}
{}

/**
 * Runs the constellation service with the specified initial peers
 *
 * @param initial_peers The peers that should be initially connected to
 */
bool Constellation::Run(UriSet const &initial_peers, core::WeakRunnable const &bootstrap_monitor)
{
  Defer const cleanup{this, &Constellation::OnCleanup};

  if (OnStartup())
  {
    Defer const cleanup_lane_services{this, &Constellation::OnTearDownLaneServices};

    if (OnBringUpLaneServices())
    {
      Defer const cleanup_ext_services{this, &Constellation::OnTearDownExternalNetwork};

      ledger::GenesisFileCreator::ConsensusParameters params;
      if (OnRestorePreviousData(params))
      {
        if (OnBringUpExternalNetwork(params, initial_peers))
        {
          OnRunning(bootstrap_monitor);
        }
      }
    }
  }

  return true;
}

void Constellation::OnBlock(ledger::Block const &block)
{
  if (main_chain_service_)
  {
    main_chain_service_->BroadcastBlock(block);
  }
}

bool Constellation::OnStartup()
{
  FETCH_LOG_INFO(LOGGING_NAME, "OnStartup()");
  return true;
}

bool Constellation::OnBringUpLaneServices()
{
  // start the internal network manager
  network_manager_.Start();

  FETCH_LOG_INFO(LOGGING_NAME, "Starting shard services...");

  // configure all the lane services
  lane_services_.Setup(network_manager_, shard_cfgs_);

  // start all the lane services and wait for them to start accepting
  // connections
  lane_services_.StartInternal();

  if (!WaitForLaneServersToStart())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to start lane server instances");
    return false;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Starting shard services...complete");

  // create the internal muddle instance
  internal_muddle_ =
      muddle::CreateMuddle("ISRD", internal_identity_, network_manager_,
                           cfg_.manifest.FindExternalAddress(ServiceIdentifier::Type::CORE));

  if (!StartInternalMuddle())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to establish internal muddle connection to lane services");
    return false;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Inter-shard Identity: ", internal_muddle_->GetAddress().ToBase64());

  // start the associated services
  storage_ = std::make_shared<StorageUnitClient>(internal_muddle_->GetEndpoint(), shard_cfgs_,
                                                 cfg_.log2_num_lanes);

  lane_control_ = std::make_unique<LaneRemoteControl>(internal_muddle_->GetEndpoint(), shard_cfgs_,
                                                      cfg_.log2_num_lanes);

  return true;
}

bool Constellation::OnRestorePreviousData(ledger::GenesisFileCreator::ConsensusParameters &params)
{
  FETCH_LOG_INFO(LOGGING_NAME, "OnRestorePreviousData()");

  GenesisFileCreator::Result genesis_status = GenesisFileCreator::Result::FAILURE;

  // attempt to do of the following things
  // - perform initial start up of the system state
  // - recover from previous genesis init
  {
    GenesisFileCreator creator(*storage_, external_identity_, cfg_.db_prefix);

    genesis_status = creator.LoadContents(cfg_.genesis_file_contents, cfg_.proof_of_stake, params);

    if (genesis_status == GenesisFileCreator::Result::FAILURE)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to restore or generate Genesis configuration");
      return false;
    }
  }

  // create the DAG
  dag_ = GenerateDAG(cfg_, "dag_db_", true, external_identity_);

  // create the chain
  chain_ = std::make_unique<MainChain>(ledger::MainChain::Mode::LOAD_PERSISTENT_DB, true);

  // necessary when doing state validity checks
  execution_manager_ = std::make_shared<ExecutionManager>(
      cfg_.num_executors, cfg_.log2_num_lanes, storage_,
      [this] { return std::make_shared<Executor>(storage_); }, tx_status_cache_);

  if (!GenesisSanityChecks(genesis_status))
  {
    return false;
  }

  if (!CheckStateIntegrity())
  {
    return false;
  }

  auto const heaviest_block = chain_->GetHeaviestBlock();
  FETCH_LOG_INFO(LOGGING_NAME, "Head of chain: #", heaviest_block->block_number, " 0x",
                 heaviest_block->hash.ToHex(), " Merkle: 0x", heaviest_block->merkle_hash.ToHex());

  return true;
}

bool Constellation::OnBringUpExternalNetwork(
    ledger::GenesisFileCreator::ConsensusParameters &params, UriSet const &initial_peers)
{
  FETCH_LOG_INFO(LOGGING_NAME, "OnBringUpExternalNetwork()");

  muddle_ = muddle::CreateMuddle("IHUB", external_identity_, network_manager_,
                                 cfg_.manifest.FindExternalAddress(ServiceIdentifier::Type::CORE));

  shard_management_ = std::make_shared<ShardManagementService>(cfg_.manifest, *lane_control_,
                                                               *muddle_, cfg_.log2_num_lanes);

  // setup the consensus infrastructure
  beacon_network_ = CreateBeaconNetwork(cfg_, external_identity_, network_manager_);
  beacon_setup_ =
      CreateBeaconSetupService(cfg_, *beacon_network_, *shard_management_, external_identity_);
  beacon_ = CreateBeaconService(cfg_, *beacon_network_, external_identity_, beacon_setup_);
  stake_  = CreateStakeManager(cfg_);

  consensus_ = CreateConsensus(cfg_, stake_, beacon_setup_, beacon_, *chain_, *storage_,
                               external_identity_->identity());

  consensus_->SetWhitelist(params.whitelist);
  consensus_->SetDefaultStartTime(params.start_time);
  consensus_->SetMaxCabinetSize(params.cabinet_size);

  if (params.snapshot)
  {
    consensus_->UpdateCurrentBlock(*chain_->GetHeaviestBlock());
    consensus_->Reset(*params.snapshot);
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "No snapshot to reset consensus with.");
  }

  // Update with genesis to trigger loading any saved state
  consensus_->UpdateCurrentBlock(*chain_->CreateGenesisBlock());

  block_packer_ = std::make_unique<BlockPackingAlgorithm>(cfg_.log2_num_lanes);

  block_coordinator_ = std::make_unique<ledger::BlockCoordinator>(
      *chain_, dag_, *execution_manager_, *storage_, *block_packer_, *this, external_identity_,
      cfg_.log2_num_lanes, cfg_.num_slices, consensus_,
      std::make_unique<ledger::SynergeticExecutionManager>(
          dag_, 1u, [this]() { return std::make_shared<ledger::SynergeticExecutor>(*storage_); }));

  tx_processor_ = std::make_unique<ledger::TransactionProcessor>(
      dag_, *storage_, *block_packer_, tx_status_cache_, cfg_.processor_threads);

  agent_network_ = CreateMessengerNetwork(cfg_, external_identity_, network_manager_);

  mailbox_ = CreateMessengerMailbox(cfg_, agent_network_);

  messenger_api_ = CreateMessengerAPI(cfg_, agent_network_, mailbox_);

  http_open_api_module_ = std::make_shared<OpenAPIHttpModule>();
  health_check_module_  = std::make_shared<HealthCheckHttpModule>(*chain_, *block_coordinator_);
  http_modules_         = {
      http_open_api_module_,
      health_check_module_,
      std::make_shared<p2p::P2PHttpInterface>(
          cfg_.log2_num_lanes, *chain_, *block_packer_,
          p2p::P2PHttpInterface::WeakStateMachines{block_coordinator_->GetWeakStateMachine()}),
      std::make_shared<ledger::TxStatusHttpInterface>(tx_status_cache_),
      std::make_shared<ledger::TxQueryHttpInterface>(*storage_),
      std::make_shared<ledger::ContractHttpInterface>(*storage_, *tx_processor_),
      std::make_shared<LoggingHttpModule>(),
      std::make_shared<TelemetryHttpModule>(),
      std::make_shared<MuddleStatusModule>()};

  http_ = std::make_unique<HttpServer>(http_network_manager_);
  // Display "/"
  http_->AddDefaultRootModule();

  // print the start up log banner
  FETCH_LOG_INFO(LOGGING_NAME, "Constellation :: ", cfg_.num_lanes(), "x", cfg_.num_slices, "x",
                 cfg_.num_executors);
  FETCH_LOG_INFO(LOGGING_NAME,
                 "              :: ", Address::FromMuddleAddress(muddle_->GetAddress()).display());
  FETCH_LOG_INFO(LOGGING_NAME, "              :: ", muddle_->GetAddress().ToBase64());
  FETCH_LOG_INFO(LOGGING_NAME, "");

  // Configure the cache tables
  muddle_->SetPeerTableFile(cfg_.ihub_peer_cache);
  if (beacon_network_)
  {
    beacon_network_->SetPeerTableFile(cfg_.beacon_peer_cache);
    auto beacon_conf = muddle::TrackerConfiguration::AllOn();

    beacon_conf.max_kademlia_connections  = 0;
    beacon_conf.max_longrange_connections = 0;
    beacon_network_->SetTrackerConfiguration(beacon_conf);
    beacon_network_->SetPeerTableFile(cfg_.beacon_peer_cache);
  }

  // Adding agent http inteface if network exists
  if (agent_network_)
  {
    http_modules_.push_back(std::make_shared<messenger::MessengerHttpModule>(*messenger_api_));
  }

  if (cfg_.kademlia_routing)
  {
    muddle_->SetTrackerConfiguration(muddle::TrackerConfiguration::AllOn());
  }

  // Enable experimental features
  if (cfg_.features.IsEnabled("synergetic") && dag_)
  {
    dag_service_ = std::make_shared<ledger::DAGService>(muddle_->GetEndpoint(), dag_);
    reactor_.Attach(dag_service_->GetWeakRunnable());
  }

  if (cfg_.features.IsEnabled("synergetic") && dag_)
  {
    auto syn_miner = std::make_unique<NaiveSynergeticMiner>(dag_, *storage_, external_identity_);
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
    reactor_dkg_.Attach(beacon_setup_->GetWeakRunnables());
    reactor_dkg_.Attach(beacon_->GetWeakRunnable());
  }

  // attach the services to the reactor
  reactor_.Attach(shard_management_);

  // configure the middleware of the http server
  http_->AddMiddleware(http::middleware::AllowOrigin("*"));
  http_->AddMiddleware(http::middleware::Telemetry());

  // attach all the modules to the http server
  for (auto const &module : http_modules_)
  {
    http_->AddModule(*module);
  }

  http_open_api_module_->Reset(http_.get());
  network_manager_.Start();
  http_network_manager_.Start();

  // always use mapping based ports
  muddle::MuddleInterface::PortMapping port_mapping{};
  port_mapping[p2p_port_] = LookupRemotePort(cfg_.manifest, ServiceIdentifier::Type::CORE);

  muddle_->Start(initial_peers, port_mapping);

  // beacon network
  if (beacon_network_)
  {
    uint16_t const beacon_bind_port = LookupLocalPort(cfg_.manifest, ServiceIdentifier::Type::DKG);
    uint16_t const beacon_ext_port  = LookupRemotePort(cfg_.manifest, ServiceIdentifier::Type::DKG);

    muddle::MuddleInterface::PortMapping const beacon_port_mapping{
        {beacon_bind_port, beacon_ext_port}};

    beacon_network_->Start({}, beacon_port_mapping);
  }

  // Adding agent network if it is enabled
  if (agent_network_)
  {
    uint16_t const agents_bind_port =
        LookupLocalPort(cfg_.manifest, ServiceIdentifier::Type::AGENTS);
    uint16_t const agents_ext_port =
        LookupRemotePort(cfg_.manifest, ServiceIdentifier::Type::AGENTS);

    muddle::MuddleInterface::PortMapping const agents_port_mapping{
        {agents_bind_port, agents_ext_port}};

    agent_network_->Start({}, agents_port_mapping);
  }

  // reactor important to run the block/chain state machine
  reactor_.Start();
  reactor_dkg_.Start();

  /// BLOCK EXECUTION & MINING
  execution_manager_->Start();
  tx_processor_->Start();

  // create the main chain service (from this point it will be able to start accepting) external
  // requests
  main_chain_rpc_client_ = std::make_shared<ledger::MainChainRpcClient>(muddle_->GetEndpoint());
  main_chain_service_    = std::make_shared<MainChainRpcService>(
      muddle_->GetEndpoint(), *main_chain_rpc_client_, *chain_, trust_, consensus_);

  // the health check module needs the latest chain service
  health_check_module_->UpdateChainService(*main_chain_service_);

  /// INPUT INTERFACES

  // Finally start the HTTP server
  http_->Start(http_port_);

  // Start the main syncing state machine for main chain service
  reactor_.Attach(main_chain_service_->GetWeakRunnable());

  // The block coordinator needs to access correctly started lanes to recover state in the case of
  // a crash.
  reactor_.Attach(block_coordinator_->GetWeakRunnable());

  return true;
}

bool Constellation::OnRunning(core::WeakRunnable const &bootstrap_monitor)
{
  bool start_up_in_progress{true};

  // monitor loop
  while (active_)
  {
    // determine the status of the main chain server
    bool const is_in_sync = main_chain_service_->IsSynced() && block_coordinator_->IsSynced();

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
      reactor_.Attach(bootstrap_monitor);
      start_up_in_progress = false;

      FETCH_LOG_INFO(LOGGING_NAME, "Startup complete");
    }

    // update the up time counter
    uptime_->increment();
  }

  return true;
}

void Constellation::OnTearDownExternalNetwork()
{
  FETCH_LOG_INFO(LOGGING_NAME, "OnTearDownExternalNetwork()");

  if (http_)
  {
    // TODO(LDGR-695): There is a logical flaw in the http server that causes
    // catastrophic failure on shutdown. The key problem has to do with
    // the order in which objects are destructed and the fact, that
    // connections are not shutdown by calling Stop.
    http_->Stop();
    ResetItem(http_);
  }

  ResetItem(main_chain_service_);
  ResetItem(main_chain_rpc_client_);

  if (tx_processor_)
  {
    tx_processor_->Stop();
  }

  if (execution_manager_)
  {
    execution_manager_->Stop();
  }

  reactor_.Stop();
  reactor_dkg_.Stop();

  if (agent_network_)
  {
    agent_network_->Stop();
  }

  if (beacon_network_)
  {
    beacon_network_->Stop();
  }

  if (muddle_)
  {
    muddle_->Stop();
  }

  lane_services_.StopExternal();

  ResetItem(synergetic_miner_);
  ResetItem(dag_service_);

  http_modules_.clear();
  ResetItem(health_check_module_);
  ResetItem(http_open_api_module_);

  ResetItem(messenger_api_);
  ResetItem(mailbox_);
  ResetItem(agent_network_);
  ResetItem(tx_processor_);
  ResetItem(block_coordinator_);
  ResetItem(block_packer_);
  ResetItem(execution_manager_);
  ResetItem(consensus_);
  ResetItem(stake_);
  ResetItem(beacon_);
  ResetItem(beacon_setup_);
  ResetItem(beacon_network_);
  ResetItem(shard_management_);
  ResetItem(muddle_);
}

void Constellation::OnTearDownLaneServices()
{
  ResetItem(chain_);
  ResetItem(lane_control_);
  ResetItem(storage_);

  // shutdown the internal muddle
  if (internal_muddle_)
  {
    internal_muddle_->Stop();
    internal_muddle_.reset();
  }

  // tear down the lane services
  lane_services_.StopInternal();
}

void Constellation::OnCleanup()
{}

bool Constellation::StartInternalMuddle()
{
  using Peers     = muddle::MuddleInterface::Peers;
  using Clock     = std::chrono::steady_clock;
  using Timestamp = Clock::time_point;

  // build the complete list of Uris to all the lane services across the internal network
  Peers internal_peers{};
  for (auto const &shard : shard_cfgs_)
  {
    internal_peers.emplace("tcp://127.0.0.1:" + std::to_string(shard.internal_port));
  }

  // start the muddle up and connect to all the shards
  internal_muddle_->Start(internal_peers, {});

  // wait for all the connections to establish
  Timestamp const deadline = Clock::now() + std::chrono::seconds{30};

  bool success{false};
  while (Clock::now() < deadline)
  {
    // exit the wait loop until all the connections have been formed
    if (internal_muddle_->GetNumDirectlyConnectedPeers() >= shard_cfgs_.size())
    {
      success = true;
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{500});
  }

  return success;
}

bool Constellation::GenesisSanityChecks(GenesisFileCreator::Result genesis_status)
{
  // lookup the heaviest block and perform some sanity checks
  auto const heaviest_block = chain_->GetHeaviestBlock();
  if (!heaviest_block)
  {
    return false;
  }

  bool const is_genesis_correct = (heaviest_block->hash == chain::GetGenesisDigest()) &&
                                  (heaviest_block->merkle_hash == chain::GetGenesisMerkleRoot());

  if (GenesisFileCreator::Result::LOADED_PREVIOUS_GENESIS == genesis_status)
  {
    if (heaviest_block->IsGenesis())
    {
      // validate the hash and merkle hash
      if (!is_genesis_correct)
      {
        FETCH_LOG_CRITICAL(LOGGING_NAME,
                           "Heaviest block recovered as start up was marked as genesis but did not "
                           "match genesis state");
        return false;
      }

      FETCH_LOG_INFO(LOGGING_NAME, "Heaviest block is genesis. That seems suspicious. Block: #",
                     heaviest_block->block_number, " 0x", heaviest_block->hash.ToHex(),
                     " Merkle: 0x", heaviest_block->merkle_hash.ToHex());
    }
  }
  else if (GenesisFileCreator::Result::CREATED_NEW_GENESIS == genesis_status)
  {
    if (!heaviest_block->IsGenesis())
    {
      FETCH_LOG_CRITICAL(
          LOGGING_NAME,
          "Recovered to initial genesis state but this is mismatched against the current chain");
      return false;
    }

    if (!is_genesis_correct)
    {
      FETCH_LOG_CRITICAL(LOGGING_NAME,
                         "Internal error, genesis block in chain does not match system genesis "
                         "digest and/or merkle digest");
      return false;
    }
  }

  return true;
}

// Check the integrity of the state database and setup some classes as if they just
// finished executing a block
bool Constellation::CheckStateIntegrity()
{
  // lookup the heaviest block and perform some sanity checks
  auto current_block     = chain_->GetHeaviestBlock();
  auto current_state     = storage_->CurrentHash();
  auto last_commit_state = storage_->LastCommitHash();

  FETCH_LOG_INFO(LOGGING_NAME, "Performing State Integrity Check:");
  FETCH_LOG_INFO(LOGGING_NAME, " - Current: 0x", current_state.ToHex());
  FETCH_LOG_INFO(LOGGING_NAME, " - Last Commit: 0x", last_commit_state.ToHex());
  FETCH_LOG_INFO(LOGGING_NAME, " - Merkle State: 0x", current_block->merkle_hash.ToHex());

  if (current_block->IsGenesis())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "The main chain's heaviest is genesis. Nothing to do.");
    return true;
  }

  // Walk back down the chain until we find a state we could revert to
  while (current_block &&
         !storage_->HashExists(current_block->merkle_hash, current_block->block_number))
  {
    current_block = chain_->GetBlock(current_block->previous_hash);
  }

  if (!current_block)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to walk back the chain when verifying initial state!");
    return false;
  }

  if (current_block &&
      storage_->HashExists(current_block->merkle_hash, current_block->block_number))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Found a block to revert to! Block: ", current_block->block_number,
                   " hex: 0x", current_block->hash.ToHex(), " merkle hash: 0x",
                   current_block->merkle_hash.ToHex());

    if (!storage_->RevertToHash(current_block->merkle_hash, current_block->block_number))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "The revert operation failed!");
      return false;
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Reverted storage unit.");
    FETCH_LOG_INFO(LOGGING_NAME, "Reverting DAG to: ", current_block->block_number);

    // Need to revert the DAG too
    if (dag_ && !dag_->RevertToEpoch(current_block->block_number))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Reverting the DAG failed!");
      return false;
    }

    // we need to update the execution manager state and also our locally cached state about the
    // 'last' block that has been executed
    execution_manager_->SetLastProcessedBlock(current_block->hash);
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Didn't find any prior merkle state to revert to.");
    return false;
  }

  return true;
}

void Constellation::SignalStop()
{
  active_ = false;
}

std::ostream &operator<<(std::ostream &stream, Constellation::Config const &config)
{
  stream << "Network Mode.........: " << ToString(config.network_mode) << '\n';
  stream << "Num Lanes............: " << config.num_lanes() << '\n';
  stream << "Num Slices...........: " << config.num_slices << '\n';
  stream << "Num Executors........: " << config.num_executors << '\n';
  stream << "DB Prefix............: " << config.num_executors << '\n';
  stream << "Processor Threads....: " << config.processor_threads << '\n';
  stream << "Verification Threads.: " << config.verification_threads << '\n';
  stream << "Max Peers............: " << config.max_peers << '\n';
  stream << "Transient Peers......: " << config.transient_peers << '\n';
  stream << "Block Internal.......: " << config.block_interval_ms << "ms\n";
  stream << "Max Cabinet Size.....: " << config.max_cabinet_size << '\n';
  stream << "Stake Delay Period...: " << config.stake_delay_period << '\n';
  stream << "Aeon Period..........: " << config.aeon_period << '\n';
  stream << "Kad Routing..........: " << config.kademlia_routing << '\n';
  stream << "Proof of Stake.......: " << config.proof_of_stake << '\n';
  stream << "Agents...............: " << config.enable_agents << '\n';
  stream << "Messenger Port.......: " << config.messenger_port << '\n';
  stream << "Mailbox Port.........: " << config.mailbox_port << '\n';

  return stream;
}

}  // namespace constellation
}  // namespace fetch
