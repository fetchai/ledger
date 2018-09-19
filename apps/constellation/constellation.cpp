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
#include "ledger/chaincode/wallet_http_interface.hpp"
#include "ledger/execution_manager.hpp"
#include "network/p2pservice/explore_http_interface.hpp"
#include "network/p2pservice/p2p_http_interface.hpp"
#include "ledger/storage_unit/lane_remote_control.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/uri.hpp"
#include "ledger/chaincode/wallet_http_interface.hpp"
#include "network/p2pservice/p2p_http_interface.hpp"
#include "http/middleware/allow_origin.hpp"

#include <memory>
#include <random>

using fetch::byte_array::ToBase64;
using fetch::ledger::Executor;
using fetch::network::TCPClient;

using ExecutorPtr = std::shared_ptr<Executor>;

namespace fetch {
namespace {

  std::size_t CalcNetworkManagerThreads(std::size_t num_lanes)
  {
    static constexpr std::size_t THREADS_PER_LANE = 2;
    static constexpr std::size_t OTHER_THREADS = 10;

    return (num_lanes * THREADS_PER_LANE) + OTHER_THREADS;
  }

} // namespace

/**
 * Construct a constellation instance
 *
 * @param certificate The reference to the node public key
 * @param port_start The start port for all the services
 * @param num_executors The number of executors
 * @param num_lanes The configured number of lanes
 * @param num_slices The configured number of slices
 * @param interface_address The current interface address TODO(EJF): This should be more integrated
 * @param db_prefix The database file(s) prefix
 */
Constellation::Constellation(CertificatePtr &&certificate, uint16_t port_start,
                             uint32_t num_executors, uint32_t log2_num_lanes,
                             uint32_t num_slices, std::string const &interface_address,
                             std::string const &db_prefix)
  : active_{true}
  , interface_address_{interface_address}
  , num_lanes_{static_cast<uint32_t>(1u << log2_num_lanes)}
  , num_slices_{static_cast<uint32_t>(num_slices)}
  , p2p_port_{static_cast<uint16_t>(port_start + P2P_PORT_OFFSET)}
  , http_port_{static_cast<uint16_t>(port_start + HTTP_PORT_OFFSET)}
  , lane_port_start_{static_cast<uint16_t>(port_start + STORAGE_PORT_OFFSET)}
  , main_chain_port_{static_cast<uint16_t>(port_start + MAIN_CHAIN_PORT_OFFSET)}
  , network_manager_{CalcNetworkManagerThreads(num_lanes_)}
  , muddle_{std::move(certificate), network_manager_}
  , trust_{}
  , p2p_{muddle_, lane_control_, trust_}
  , lane_services_()
  , storage_(std::make_shared<StorageUnitClient>(network_manager_))
  , lane_control_(num_lanes_)
  , execution_manager_{
    std::make_shared<ExecutionManager>(
      num_executors,
      storage_,
      [this] {
        return std::make_shared<Executor>(storage_);
      }
    )
  }
  , chain_{}
  , block_packer_{log2_num_lanes, num_slices}
  , block_coordinator_{chain_, *execution_manager_}
  , miner_{num_lanes_, num_slices, chain_, block_coordinator_, block_packer_, p2p_port_} // p2p_port_ fairly arbitrary
  , main_chain_service_{std::make_shared<MainChainRpcService>(p2p_.AsEndpoint(), chain_, trust_)}
  , tx_processor_{*storage_, block_packer_}
  , http_{network_manager_}
  , http_modules_{
    std::make_shared<ledger::WalletHttpInterface>(*storage_, tx_processor_),
    std::make_shared<p2p::P2PHttpInterface>(chain_, muddle_, p2p_, trust_)
  }
{
  FETCH_UNUSED(num_slices_);

  // print the start up log banner
  FETCH_LOG_INFO(LOGGING_NAME,"Constellation :: ", interface_address, " P ", port_start, " E ",
                     num_executors, " S ", num_lanes_, "x", num_slices);
  FETCH_LOG_INFO(LOGGING_NAME,"              :: ", ToBase64(p2p_.identity().identifier()));
  FETCH_LOG_INFO(LOGGING_NAME,"");

  miner_.OnBlockComplete([this](auto const &block) {
    main_chain_service_->BroadcastBlock(block);
  });

  // configure all the lane services
  lane_services_.Setup(db_prefix, num_lanes_, lane_port_start_, network_manager_);

  // configure the middleware of the http server
  http_.AddMiddleware(
    http::middleware::AllowOrigin("*")
  );

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

  // start all the services
  network_manager_.Start();
  muddle_.Start({p2p_port_});

  lane_services_.Start();

  // add the lane connections
  storage_->SetNumberOfLanes(num_lanes_);
  for (uint32_t i = 0; i < num_lanes_; ++i)
  {
    uint16_t const lane_port = static_cast<uint16_t>(lane_port_start_ + i);

    // establish the connection to the lane
    auto client = storage_->AddLaneConnection<TCPClient>("127.0.0.1", lane_port);

    // allow the remote control to use connection
    lane_control_.AddClient(i, client);
  }
  execution_manager_->Start();
  block_coordinator_.Start();

  if (mining)
    miner_.Start();

  // P2P configuration
  p2p_.SetLocalManifest(GenerateManifest());
  p2p_.Start(initial_peers, network::Uri("rpc://127.0.0.1:" + std::to_string(p2p_port_)) );

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

  // tear down all the services
  if (mining)
    miner_.Stop();

  block_coordinator_.Stop();
  execution_manager_->Stop();
  lane_services_.Stop();
  p2p_ . Stop();
  muddle_ . Stop();
  network_manager_.Stop();

  FETCH_LOG_INFO(LOGGING_NAME, "Shutting down...complete");
}

Constellation::Manifest Constellation::GenerateManifest() const
{
  std::string my_manifest = "MAINCHAIN   0     tcp://127.0.0.1:" + std::to_string(main_chain_port_) + "\n";

  for (uint32_t i = 0; i < num_lanes_; ++i)
  {
    uint16_t const lane_port = static_cast<uint16_t>(lane_port_start_ + i);
    my_manifest += "LANE     " + std::to_string(i) + "     " + "tcp://127.0.0.1:" + std::to_string(lane_port) + "\n";
  }

  return Manifest::FromText(my_manifest);
}

}  // namespace fetch
