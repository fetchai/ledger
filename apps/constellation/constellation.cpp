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
#include "http/middleware/allow_origin.hpp"
#include "ledger/chain/consensus/bad_miner.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"
#include "ledger/chaincode/contract_http_interface.hpp"
#include "ledger/chaincode/wallet_http_interface.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/storage_unit/lane_remote_control.hpp"
#include "ledger/tx_status_http_interface.hpp"
#include "network/generics/atomic_inflight_counter.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/p2pservice/p2p_http_interface.hpp"
#include "network/uri.hpp"

#include <memory>
#include <random>
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

using ExecutorPtr = std::shared_ptr<Executor>;
using ConsensusMinerInterfacePtr =
    std::shared_ptr<fetch::ledger::consensus::ConsensusMinerInterface>;

namespace fetch {
namespace {

using LaneIndex = fetch::ledger::LaneIdentity::lane_type;

// static const std::chrono::milliseconds LANE_CONNECTION_TIME{10000};
static const std::size_t HTTP_THREADS{4};

bool WaitForLaneServersToStart()
{
  using InFlightCounter = AtomicInFlightCounter<AtomicCounterName::TCP_PORT_STARTUP>;

  network::FutureTimepoint const deadline(std::chrono::seconds(30));

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

    auto const &ext_identity = cfg.external_identity->identity().identifier();
    auto const &int_identity = cfg.internal_identity->identity().identifier();

    FETCH_LOG_INFO(Constellation::LOGGING_NAME, "Shard ", i + 1);
    FETCH_LOG_INFO(Constellation::LOGGING_NAME, " - Internal ", ToBase64(int_identity), " - ",
                   cfg.internal_network_id.ToString(), " - tcp://0.0.0.0:", cfg.internal_port);
    FETCH_LOG_INFO(Constellation::LOGGING_NAME, " - External ", ToBase64(ext_identity), " - ",
                   cfg.external_network_id.ToString(), " - tcp://0.0.0.0:", cfg.external_port);
  }

  return configs;
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
Constellation::Constellation(CertificatePtr &&certificate, Config config)
  : active_{true}
  , cfg_{std::move(config)}
  , p2p_port_(LookupLocalPort(cfg_.manifest, ServiceType::CORE))
  , http_port_(LookupLocalPort(cfg_.manifest, ServiceType::HTTP))
  , lane_port_start_(LookupLocalPort(cfg_.manifest, ServiceType::LANE))
  , shard_cfgs_{GenerateShardsConfig(config.num_lanes(), lane_port_start_, cfg_.db_prefix)}
  , reactor_{"Reactor"}
  , network_manager_{"NetMgr", CalcNetworkManagerThreads(cfg_.num_lanes())}
  , http_network_manager_{"Http", HTTP_THREADS}
  , muddle_{muddle::NetworkId{"IHUB"}, certificate, network_manager_}
  , internal_identity_{std::make_shared<crypto::ECDSASigner>()}
  , internal_muddle_{muddle::NetworkId{"ISRD"}, internal_identity_, network_manager_}
  , trust_{}
  , p2p_{muddle_,        lane_control_,        trust_,
         cfg_.max_peers, cfg_.transient_peers, cfg_.peers_update_cycle_ms}
  , lane_services_()
  , storage_(std::make_shared<StorageUnitClient>(internal_muddle_.AsEndpoint(), shard_cfgs_,
                                                 cfg_.log2_num_lanes))
  , lane_control_(internal_muddle_.AsEndpoint(), shard_cfgs_, cfg_.log2_num_lanes)
  , execution_manager_{std::make_shared<ExecutionManager>(
        cfg_.num_executors, storage_, [this] { return std::make_shared<Executor>(storage_); })}
  , chain_{ledger::MainChain::Mode::LOAD_PERSISTENT_DB}
  , block_packer_{cfg_.log2_num_lanes, cfg_.num_slices}
  , block_coordinator_{chain_,
                       *execution_manager_,
                       *storage_,
                       block_packer_,
                       *this,
                       tx_status_cache_,
                       muddle_.identity().identifier(),
                       cfg_.num_lanes(),
                       cfg_.num_slices,
                       cfg_.block_difficulty}
  , main_chain_service_{std::make_shared<MainChainRpcService>(p2p_.AsEndpoint(), chain_, trust_,
                                                              cfg_.standalone)}
  , tx_processor_{*storage_, block_packer_, tx_status_cache_, cfg_.processor_threads}
  , http_{http_network_manager_}
  , http_modules_{
        std::make_shared<ledger::WalletHttpInterface>(*storage_, tx_processor_, cfg_.num_lanes()),
        std::make_shared<p2p::P2PHttpInterface>(cfg_.log2_num_lanes, chain_, muddle_, p2p_, trust_,
                                                block_packer_),
        std::make_shared<ledger::TxStatusHttpInterface>(tx_status_cache_),
        std::make_shared<ledger::ContractHttpInterface>(*storage_, tx_processor_)}
{
  // print the start up log banner
  FETCH_LOG_INFO(LOGGING_NAME, "Constellation :: ", cfg_.interface_address, " E ",
                 cfg_.num_executors, " S ", cfg_.num_lanes(), "x", cfg_.num_slices);
  FETCH_LOG_INFO(LOGGING_NAME, "              :: ", ToBase64(p2p_.identity().identifier()));
  FETCH_LOG_INFO(LOGGING_NAME, "");

  // attach the services to the reactor
  reactor_.Attach(block_coordinator_.GetWeakRunnable());
  reactor_.Attach(main_chain_service_->GetWeakRunnable());

  // configure all the lane services
  lane_services_.Setup(network_manager_, shard_cfgs_);

  // configure the middleware of the http server
  http_.AddMiddleware(http::middleware::AllowOrigin("*"));

  // attach all the modules to the http server
  for (auto const &module : http_modules_)
  {
    http_.AddModule(*module);
  }

  CreateInfoFile("info.json");
}

void Constellation::CreateInfoFile(std::string const &filename)
{
  // Create an information file about this process.

  std::fstream stream;
  stream.open(filename.c_str(), std::ios_base::out);
  if (stream.good())
  {
    variant::Variant data = variant::Variant::Object();
    data["pid"]           = getpid();
    data["identity"]      = fetch::byte_array::ToBase64(muddle_.identity().identifier());
    data["hex_identity"]  = fetch::byte_array::ToHex(muddle_.identity().identifier());

    stream << data;

    stream.close();
  }
  else
  {
    throw std::invalid_argument(std::string("Can't open ") + filename);
  }
}

/**
 * Runs the constellation service with the specified initial peers
 *
 * @param initial_peers The peers that should be initially connected to
 */
void Constellation::Run(UriList const &initial_peers)
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

  //---------------------------------------------------------------
  // Step 2. Main monitor loop
  //---------------------------------------------------------------

  // monitor loop
  while (active_)
  {
    // control from the top level block production based on the chain sync state
    block_coordinator_.EnableMining(ledger::MainChainRpcService::State::SYNCHRONISED ==
                                    main_chain_service_->state());

    FETCH_LOG_DEBUG(LOGGING_NAME, "Still alive...");
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
  }

  //---------------------------------------------------------------
  // Step 3. Tear down
  //---------------------------------------------------------------

  FETCH_LOG_INFO(LOGGING_NAME, "Shutting down...");

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
