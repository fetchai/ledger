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

#include "constellation.hpp"
#include "health_check_http_module.hpp"
#include "http/middleware/allow_origin.hpp"
#include "ledger/chain/consensus/bad_miner.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"
#include "logging_http_module.hpp"

#include "ledger/chaincode/contract_http_interface.hpp"
#include "ledger/dag/dag_interface.hpp"

#include "ledger/consensus/naive_entropy_generator.hpp"
#include "ledger/consensus/stake_snapshot.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/storage_unit/lane_remote_control.hpp"
#include "ledger/tx_query_http_interface.hpp"
#include "ledger/tx_status_http_interface.hpp"
#include "network/generics/atomic_inflight_counter.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/p2pservice/p2p_http_interface.hpp"
#include "network/uri.hpp"
#include "dkg/dkg_service.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>

using fetch::byte_array::ToBase64;
using fetch::ledger::Executor;
using fetch::network::Manifest;
using fetch::network::ServiceType;
using fetch::network::Uri;
using fetch::network::ServiceIdentifier;
using fetch::network::AtomicInFlightCounter;
using fetch::network::AtomicCounterName;
using fetch::network::Uri;
using fetch::network::Peer;
using fetch::ledger::Address;
using fetch::ledger::GenesisFileCreator;
using fetch::muddle::MuddleEndpoint;

using ExecutorPtr = std::shared_ptr<Executor>;

namespace fetch {
namespace {

using LaneIndex       = fetch::ledger::LaneIdentity::lane_type;
using StakeManagerPtr = std::shared_ptr<ledger::StakeManager>;
using EntropyPtr      = std::unique_ptr<ledger::EntropyGeneratorInterface>;
using DkgServicePtr   = std::unique_ptr<dkg::DkgService>;
using ConstByteArray  = byte_array::ConstByteArray;

static const std::size_t HTTP_THREADS{4};
static char const *      SNAPSHOT_FILENAME = "snapshot.json";

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

uint16_t LookupLocalPort(Manifest const &manifest, ServiceType service, uint16_t instance = 0)
{
  ServiceIdentifier identifier{service, instance};

  if (!manifest.HasService(identifier))
  {
    throw std::runtime_error("Unable to lookup requested service from the manifest");
  }

  return manifest.GetLocalPort(identifier);
}

std::shared_ptr<ledger::DAGInterface> GenerateDAG(bool generate, std::string const &db_name,
                                                  bool                          load_on_start,
                                                  Constellation::CertificatePtr certificate)
{
  if (generate)
  {
    return std::make_shared<ledger::DAG>(db_name, load_on_start, certificate);
  }

  return nullptr;
}

ledger::ShardConfigs GenerateShardsConfig(uint32_t num_lanes, uint16_t start_port,
                                          std::string const &storage_path)
{
  ledger::ShardConfigs configs(num_lanes);

  for (uint32_t i = 0; i < num_lanes; ++i)
  {
    auto &cfg = configs[i];

    cfg.lane_id           = i;
    cfg.num_lanes         = num_lanes;
    cfg.storage_path      = storage_path;
    cfg.external_identity = std::make_shared<crypto::ECDSASigner>();
    cfg.external_port     = start_port++;
    cfg.external_network_id =
        muddle::NetworkId{(static_cast<uint32_t>(i) & 0xFFFFFFu) | (uint32_t{'L'} << 24u)};
    cfg.internal_identity   = std::make_shared<crypto::ECDSASigner>();
    cfg.internal_port       = start_port++;
    cfg.internal_network_id = muddle::NetworkId{"ISRD"};

    auto const ext_identity = cfg.external_identity->identity().identifier();
    auto const int_identity = cfg.internal_identity->identity().identifier();

    FETCH_LOG_INFO(Constellation::LOGGING_NAME, "Shard ", i + 1);
    FETCH_LOG_INFO(Constellation::LOGGING_NAME, " - Internal ", ToBase64(int_identity), " - ",
                   cfg.internal_network_id.ToString(), " - tcp://0.0.0.0:", cfg.internal_port);
    FETCH_LOG_INFO(Constellation::LOGGING_NAME, " - External ", ToBase64(ext_identity), " - ",
                   cfg.external_network_id.ToString(), " - tcp://0.0.0.0:", cfg.external_port);
  }

  return configs;
}

EntropyPtr CreateEntropy()
{
  return std::make_unique<ledger::NaiveEntropyGenerator>();
}

StakeManagerPtr CreateStakeManager(bool enabled, ledger::EntropyGeneratorInterface &entropy)
{
  StakeManagerPtr mgr{};

  if (enabled)
  {
    mgr = std::make_shared<ledger::StakeManager>(entropy);
  }

  return mgr;
}

DkgServicePtr CreateDkgService(Constellation::Config const &cfg, ConstByteArray address, MuddleEndpoint &endpoint)
{
  DkgServicePtr dkg{};

  if (cfg.proof_of_stake && !cfg.beacon_address.empty())
  {
    // !!! - Genuinely terrifying
    crypto::bls::Init();

    // TODO(EJF): Move key lifetime into block
    dkg = std::make_unique<dkg::DkgService>(endpoint, address, cfg.beacon_address, 200);
  }

  return dkg;
}

}  // namespace

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
  , p2p_port_(LookupLocalPort(cfg_.manifest, ServiceType::CORE))
  , http_port_(LookupLocalPort(cfg_.manifest, ServiceType::HTTP))
  , lane_port_start_(LookupLocalPort(cfg_.manifest, ServiceType::LANE))
  , shard_cfgs_{GenerateShardsConfig(config.num_lanes(), lane_port_start_, cfg_.db_prefix)}
  , reactor_{"Reactor"}
  , network_manager_{"NetMgr", CalcNetworkManagerThreads(cfg_.num_lanes())}
  , http_network_manager_{"Http", HTTP_THREADS}
  , muddle_{muddle::NetworkId{"IHUB"}, certificate, network_manager_, !config.disable_signing,
            config.sign_broadcasts}
  , internal_identity_{std::make_shared<crypto::ECDSASigner>()}
  , internal_muddle_{muddle::NetworkId{"ISRD"}, internal_identity_, network_manager_}
  , trust_{}
  , p2p_{muddle_,        lane_control_,        trust_,
         cfg_.max_peers, cfg_.transient_peers, cfg_.peers_update_cycle_ms}
  , lane_services_()
  , storage_(std::make_shared<StorageUnitClient>(internal_muddle_.AsEndpoint(), shard_cfgs_,
                                                 cfg_.log2_num_lanes))
  , lane_control_(internal_muddle_.AsEndpoint(), shard_cfgs_, cfg_.log2_num_lanes)
  , dag_{GenerateDAG(cfg_.features.IsEnabled("synergetic"), "dag_db_", true, certificate)}
  , dkg_{CreateDkgService(cfg_, certificate->identity().identifier(), muddle_.AsEndpoint())}
  , entropy_{CreateEntropy()}
  , stake_{CreateStakeManager(cfg_.proof_of_stake, *entropy_)}
  , execution_manager_{std::make_shared<ExecutionManager>(
        cfg_.num_executors, cfg_.log2_num_lanes, storage_,
        [this] {
          return std::make_shared<Executor>(storage_, stake_ ? &stake_->update_queue() : nullptr);
        })}
  , chain_{ledger::MainChain::Mode::LOAD_PERSISTENT_DB}
  , block_packer_{cfg_.log2_num_lanes}
  , block_coordinator_{chain_,
                       dag_,
                       stake_,
                       *execution_manager_,
                       *storage_,
                       block_packer_,
                       *this,
                       tx_status_cache_,
                       cfg_.features,
                       certificate,
                       cfg_.num_lanes(),
                       cfg_.num_slices,
                       cfg_.block_difficulty}
  , main_chain_service_{std::make_shared<MainChainRpcService>(p2p_.AsEndpoint(), chain_, trust_,
                                                              cfg_.network_mode)}
  , tx_processor_{dag_, *storage_, block_packer_, tx_status_cache_, cfg_.processor_threads}
  , http_{http_network_manager_}
  , http_modules_{
        std::make_shared<p2p::P2PHttpInterface>(
            cfg_.log2_num_lanes, chain_, muddle_, p2p_, trust_, block_packer_,
            p2p::P2PHttpInterface::WeakStateMachines{main_chain_service_->GetWeakStateMachine(),
                                                     block_coordinator_.GetWeakStateMachine()}),
        std::make_shared<ledger::TxStatusHttpInterface>(tx_status_cache_),
        std::make_shared<ledger::TxQueryHttpInterface>(*storage_),
        std::make_shared<ledger::ContractHttpInterface>(*storage_, tx_processor_),
        std::make_shared<LoggingHttpModule>(),
        std::make_shared<HealthCheckHttpModule>(chain_, *main_chain_service_, block_coordinator_)}
{

  // print the start up log banner
  FETCH_LOG_INFO(LOGGING_NAME, "Constellation :: ", cfg_.interface_address, " E ",
                 cfg_.num_executors, " S ", cfg_.num_lanes(), "x", cfg_.num_slices);
  FETCH_LOG_INFO(LOGGING_NAME, "              :: ", ToBase64(p2p_.identity().identifier()));
  FETCH_LOG_INFO(LOGGING_NAME, "              :: ", Address{p2p_.identity()}.display());
  FETCH_LOG_INFO(LOGGING_NAME, "");

  // Enable experimental features
  if (cfg_.features.IsEnabled("synergetic"))
  {
    assert(dag_);
    dag_service_ = std::make_shared<ledger::DAGService>(muddle_.AsEndpoint(), dag_);
    reactor_.Attach(dag_service_->GetWeakRunnable());

    NaiveSynergeticMiner *syn_miner = new NaiveSynergeticMiner{dag_, *storage_, certificate};
    reactor_.Attach(syn_miner->GetWeakRunnable());

    synergetic_miner_.reset(syn_miner);
  }

  // attach the services to the reactor
  reactor_.Attach(main_chain_service_->GetWeakRunnable());

  // configure all the lane services
  lane_services_.Setup(network_manager_, shard_cfgs_, !config.disable_signing);

  // configure the middleware of the http server
  http_.AddMiddleware(http::middleware::AllowOrigin("*"));

  // attach all the modules to the http server
  for (auto const &module : http_modules_)
  {
    http_.AddModule(*module);
  }

  // TODO(EJF): Work around
  if (dkg_)
  {
    stake_->UpdateEntropy(*dkg_);
  }

}

/**
 * Runs the constellation service with the specified initial peers
 *
 * @param initial_peers The peers that should be initially connected to
 */
void Constellation::Run(UriList const &initial_peers, core::WeakRunnable bootstrap_monitor)
{
  //---------------------------------------------------------------
  // Step 1. Start all the components
  //---------------------------------------------------------------

  // if a non-zero block interval it set then the application will generate blocks
  if (cfg_.block_interval_ms > 0)
  {
    block_coordinator_.SetBlockPeriod(std::chrono::milliseconds{cfg_.block_interval_ms});
  }

  /// NETWORKING INFRASTRUCTURE

  // start all the services
  network_manager_.Start();

  http_network_manager_.Start();
  muddle_.Start({p2p_port_});
  /// LANE / SHARD SERVERS

  // start all the lane services and wait for them to start accepting
  // connections
  lane_services_.Start();
  FETCH_LOG_INFO(LOGGING_NAME, "Starting shard services...");
  if (!WaitForLaneServersToStart())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to start lane server instances");
    return;
  }
  FETCH_LOG_INFO(LOGGING_NAME, "Starting shard services...complete");

  /// LANE / SHARD CLIENTS

  {
    FETCH_LOG_INFO(LOGGING_NAME,
                   "Inter-shard Identity: ", ToBase64(internal_muddle_.identity().identifier()));

    // build the complete list of Uris to all the lane services across the internal network
    Muddle::UriList uris;
    uris.reserve(shard_cfgs_.size());

    for (auto const &shard : shard_cfgs_)
    {
      uris.emplace_back(Uri{Peer{"127.0.0.1", shard.internal_port}});
    }

    // start the muddle up and connect to all the shards
    internal_muddle_.Start({}, uris);

    for (;;)
    {
      auto const clients = internal_muddle_.GetConnections(true);

      // exit the wait loop until all the connections have been formed
      if (clients.size() >= shard_cfgs_.size())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Internal muddle network established between shards");

        for (auto const &client : clients)
        {
          FETCH_LOG_INFO(LOGGING_NAME, " - Connected to: ", ToBase64(client.first), " (",
                         client.second.ToString(), ")");
        }

        break;
      }
      else
      {
        FETCH_LOG_DEBUG(LOGGING_NAME,
                        "Waiting for internal muddle connection to be established...");
      }

      std::this_thread::sleep_for(std::chrono::milliseconds{500});
    }
  }

  // BEFORE the block coordinator starts its state set up special genesis
  if (cfg_.load_state_file)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Loading from genesis save file.");

    GenesisFileCreator creator(block_coordinator_, *storage_, stake_.get());
    creator.LoadFile(SNAPSHOT_FILENAME);

    FETCH_LOG_INFO(LOGGING_NAME, "Loaded from genesis save file.");
  }

  // reactor important to run the block/chain state machine
  reactor_.Start();

  /// BLOCK EXECUTION & MINING

  execution_manager_->Start();
  tx_processor_.Start();

  /// P2P (TRUST) HIGH LEVEL MANAGEMENT

  // P2P configuration
  p2p_.SetLocalManifest(cfg_.manifest);
  p2p_.Start(initial_peers);

  /// INPUT INTERFACES

  // Finally start the HTTP server
  http_.Start(http_port_);

  // The block coordinator needs to access correctly started lanes to recover state in the case of
  // a crash.
  reactor_.Attach(block_coordinator_.GetWeakRunnable());

  //---------------------------------------------------------------
  // Step 2. Main monitor loop
  //---------------------------------------------------------------
  bool start_up_in_progress{true};
  bool dkg_attached{false};

  // monitor loop
  while (active_)
  {
    // wait for at least one connected peer
    if (!muddle_.AsEndpoint().GetDirectlyConnectedPeers().empty())
    {
      if (dkg_ && !dkg_attached)
      {
        reactor_.Attach(dkg_->GetWeakRunnable());
        dkg_attached = true;
      }
    }

    // determine the status of the main chain server
    bool const is_in_sync = main_chain_service_->IsSynced() && block_coordinator_.IsSynced();

    // control from the top level block production based on the chain sync state
    block_coordinator_.EnableMining(is_in_sync);

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
  }

  //---------------------------------------------------------------
  // Step 3. Tear down
  //---------------------------------------------------------------

  FETCH_LOG_INFO(LOGGING_NAME, "Shutting down...");

  if (cfg_.dump_state_file)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Creating genesis save file.");

    GenesisFileCreator creator(block_coordinator_, *storage_, stake_.get());
    creator.CreateFile(SNAPSHOT_FILENAME);
  }

  http_.Stop();
  p2p_.Stop();

  tx_processor_.Stop();
  reactor_.Stop();
  execution_manager_->Stop();

  storage_.reset();

  lane_services_.Stop();
  muddle_.Stop();
  http_network_manager_.Stop();
  network_manager_.Stop();

  FETCH_LOG_INFO(LOGGING_NAME, "Shutting down...complete");
}

void Constellation::OnBlock(ledger::Block const &block)
{
  main_chain_service_->BroadcastBlock(block);
}

}  // namespace fetch
