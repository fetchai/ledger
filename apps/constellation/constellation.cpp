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
                             std::size_t num_executors, std::size_t num_lanes,
                             std::size_t num_slices, std::string const &interface_address,
                             std::string const &db_prefix)
  : active_{true}
  , interface_address_{interface_address}
  , num_lanes_{static_cast<uint32_t>(num_lanes)}
  , num_slices_{static_cast<uint32_t>(num_slices)}
  , p2p_port_{static_cast<uint16_t>(port_start + P2P_PORT_OFFSET)}
  , http_port_{static_cast<uint16_t>(port_start + HTTP_PORT_OFFSET)}
  , lane_port_start_{static_cast<uint16_t>(port_start + STORAGE_PORT_OFFSET)}
  , main_chain_port_{static_cast<uint16_t>(port_start + MAIN_CHAIN_PORT_OFFSET)}
  , network_manager_{CalcNetworkManagerThreads(num_lanes)}
  , muddle_{std::move(certificate), network_manager_}
  , p2p_{muddle_}
  , lane_services_()
  , storage_(std::make_shared<StorageUnitClient>(network_manager_))
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
  , block_packer_{}
  , block_coordinator_{chain_, *execution_manager_}
  , miner_{num_lanes, num_slices, chain_, block_coordinator_, block_packer_, p2p_port_} // p2p_port_ fairly arbitrary
  , main_chain_service_{std::make_shared<MainChainRpcService>(p2p_.AsEndpoint(), chain_)}
{
  // print the start up log banner
  FETCH_LOG_INFO(LOGGING_NAME,"Constellation :: ", interface_address, " P ", port_start, " E ",
                     num_executors, " S ", num_lanes, "x", num_slices);
  FETCH_LOG_INFO(LOGGING_NAME,"              :: ", ToBase64(p2p_.identity().identifier()));
  FETCH_LOG_INFO(LOGGING_NAME,"");

  miner_.OnBlockComplete([this](auto const &block) {
    main_chain_service_->BroadcastBlock(block);
  });

  // configure all the lane services
  lane_services_.Setup(db_prefix, num_lanes_, lane_port_start_, network_manager_);
}

/**
 * Runs the constellation service with the specified initial peers
 *
 * @param initial_peers The peers that should be initially connected to
 */
void Constellation::Run(PeerList const &initial_peers, bool mining)
{
  // start all the services
  network_manager_.Start();
  muddle_ . Start({p2p_port_});
  p2p_ . Start(initial_peers);
  lane_services_.Start();

  // add the lane connections
  storage_->SetNumberOfLanes(num_lanes_);
  for (std::size_t i = 0; i < num_lanes_; ++i)
  {
    uint16_t const lane_port = static_cast<uint16_t>(lane_port_start_ + i);

    storage_->AddLaneConnection<TCPClient>("127.0.0.1", lane_port);
  }

  execution_manager_->Start();
  block_coordinator_.Start();

  if (mining)
    miner_.Start();

  // monitor loop
  while (active_)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Still alive...");
    std::this_thread::sleep_for(std::chrono::seconds{5});
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Shutting down...");

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

}  // namespace fetch
