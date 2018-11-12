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

#include "constellation.hpp"
#include "http/middleware/allow_origin.hpp"
#include "ledger/chaincode/contract_http_interface.hpp"
#include "ledger/chaincode/wallet_http_interface.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/storage_unit/lane_remote_control.hpp"
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
using fetch::network::Peer;
using fetch::network::TCPClient;
using fetch::network::Manifest;
using fetch::network::ServiceType;
using fetch::network::Uri;
using fetch::network::ServiceIdentifier;
using fetch::network::AtomicInFlightCounter;
using fetch::network::AtomicCounterName;

using ExecutorPtr = std::shared_ptr<Executor>;

namespace fetch {
namespace {

using LaneIndex = fetch::ledger::StorageUnitClient::LaneIndex;

bool WaitForLaneServersToStart()
{
  using InFlightCounter = AtomicInFlightCounter<AtomicCounterName::TCP_PORT_STARTUP>;

  network::FutureTimepoint const deadline(std::chrono::seconds(30));

  return InFlightCounter::Wait(deadline);
}

std::size_t CalcNetworkManagerThreads(std::size_t num_lanes)
{
  static constexpr std::size_t THREADS_PER_LANE = 2;
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

std::map<LaneIndex, Peer> BuildLaneConnectionMap(Manifest const &manifest, LaneIndex num_lanes,
                                                 bool force_loopback = false)
{
  std::map<LaneIndex, Peer> connection_map;

  for (LaneIndex i = 0; i < num_lanes; ++i)
  {
    ServiceIdentifier identifier{ServiceType::LANE, static_cast<uint16_t>(i)};

    if (!manifest.HasService(identifier))
    {
      throw std::runtime_error("Unable to lookup service information from the manifest");
    }

    // lookup the service information
    auto const &service = manifest.GetService(identifier);

    // ensure the service is actually TCP based
    if (service.remote_uri.scheme() != Uri::Scheme::Tcp)
    {
      throw std::runtime_error("Non TCP connections not currently supported");
    }

    // update the connection map
    if (force_loopback)
    {
      connection_map[i] = Peer{"127.0.0.1", service.local_port};
    }
    else
    {
      connection_map[i] = service.remote_uri.AsPeer();
    }
  }

  return connection_map;
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
Constellation::Constellation(CertificatePtr &&certificate, Manifest &&manifest,
                             uint32_t num_executors, uint32_t log2_num_lanes, uint32_t num_slices,
                             std::string interface_address, std::string const &db_prefix,
                             std::string                         my_network_address,
                             std::chrono::steady_clock::duration block_interval)
  : active_{true}
  , manifest_(std::move(manifest))
  , interface_address_{std::move(interface_address)}
  , num_lanes_{static_cast<uint32_t>(1u << log2_num_lanes)}
  , num_slices_{static_cast<uint32_t>(num_slices)}
  , p2p_port_(LookupLocalPort(manifest_, ServiceType::P2P))
  , http_port_(LookupLocalPort(manifest_, ServiceType::HTTP))
  , lane_port_start_(LookupLocalPort(manifest_, ServiceType::LANE))
  , network_manager_{CalcNetworkManagerThreads(num_lanes_)}
  , http_network_manager_{4}
  , muddle_{std::move(certificate), network_manager_}
  , trust_{}
  , p2p_{muddle_, lane_control_, trust_}
  , lane_services_()
  , storage_(std::make_shared<StorageUnitClient>(network_manager_))
  , lane_control_(storage_)
  , execution_manager_{std::make_shared<ExecutionManager>(
        db_prefix, num_executors, storage_,
        [this] { return std::make_shared<Executor>(storage_); })}
  , chain_{}
  , block_packer_{log2_num_lanes, num_slices}
  , block_coordinator_{chain_, *execution_manager_}
  , miner_{num_lanes_,    num_slices, chain_,        block_coordinator_,
           block_packer_, p2p_port_,  block_interval}  // p2p_port_ fairly arbitrary
  , main_chain_service_{std::make_shared<MainChainRpcService>(p2p_.AsEndpoint(), chain_, trust_)}
  , tx_processor_{*storage_, block_packer_}
  , http_{http_network_manager_}
  , http_modules_{
        std::make_shared<ledger::WalletHttpInterface>(*storage_, tx_processor_, num_lanes_),
        std::make_shared<p2p::P2PHttpInterface>(log2_num_lanes, chain_, muddle_, p2p_, trust_, block_packer_),
        std::make_shared<ledger::ContractHttpInterface>(*storage_, tx_processor_)}
{
  FETCH_UNUSED(num_slices_);

  // print the start up log banner
  FETCH_LOG_INFO(LOGGING_NAME, "Constellation :: ", interface_address, " E ", num_executors, " S ",
                 num_lanes_, "x", num_slices);
  FETCH_LOG_INFO(LOGGING_NAME, "              :: ", ToBase64(p2p_.identity().identifier()));
  FETCH_LOG_INFO(LOGGING_NAME, "");

  miner_.OnBlockComplete([this](auto const &block) { main_chain_service_->BroadcastBlock(block); });

  // configure all the lane services
  lane_services_.Setup(db_prefix, num_lanes_, lane_port_start_, network_manager_);

  // configure the middleware of the http server
  http_.AddMiddleware(http::middleware::AllowOrigin("*"));

  // attach all the modules to the http server
  for (auto const &module : http_modules_)
  {
    http_.AddModule(*module);
  }
}

/**
 * Runs the constellation service with the specified initial peers
 *
 * @param initial_peers The peers that should be initially connected to
 */
void Constellation::Run(UriList const &initial_peers, bool mining)
{
  //---------------------------------------------------------------
  // Step 1. Start all the components
  //---------------------------------------------------------------

  /// NETWORKING INFRASTRUCTURE

  // start all the services
  network_manager_.Start();
  http_network_manager_.Start();
  muddle_.Start({p2p_port_});

  /// LANE / SHARD SERVERS

  // start all the lane services and wait for them to start accepting
  // connections
  lane_services_.Start();
  if (!WaitForLaneServersToStart())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to start lane server instances");
    return;
  }

  /// LANE / SHARD CLIENTS

  // add the lane connections
  storage_->SetNumberOfLanes(num_lanes_);
  std::size_t const count = storage_->AddLaneConnectionsWaiting<TCPClient>(
      BuildLaneConnectionMap(manifest_, num_lanes_, true), std::chrono::milliseconds(30000));

  // check to see if the connections where successful
  if (count != num_lanes_)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to establish connections to lane service");
    return;
  }

  /// BLOCK EXECUTION & MINING

  execution_manager_->Start();
  block_coordinator_.Start();
  tx_processor_.Start();

  if (mining)
  {
    miner_.Start();
  }

  /// P2P (TRUST) HIGH LEVEL MANAGEMENT

  // P2P configuration
  p2p_.SetLocalManifest(manifest_);
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
    FETCH_LOG_DEBUG(LOGGING_NAME, "Still alive...");
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
  }

  //---------------------------------------------------------------
  // Step 3. Tear down
  //---------------------------------------------------------------

  FETCH_LOG_INFO(LOGGING_NAME, "Shutting down...");

  http_.Stop();
  p2p_.Stop();

  // tear down all the services
  if (mining)
  {
    miner_.Stop();
  }

  tx_processor_.Stop();
  block_coordinator_.Stop();
  execution_manager_->Stop();

  storage_.reset();

  lane_services_.Stop();
  muddle_.Stop();
  http_network_manager_.Stop();
  network_manager_.Stop();

  FETCH_LOG_INFO(LOGGING_NAME, "Shutting down...complete");
}

}  // namespace fetch
