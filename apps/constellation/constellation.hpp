#pragma once
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

#include "http/module.hpp"
#include "http/server.hpp"
#include "ledger/chain/consensus/consensus_miner_interface.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/main_chain_miner.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/protocols/main_chain_rpc_service.hpp"
#include "ledger/storage_unit/lane_remote_control.hpp"
#include "ledger/storage_unit/storage_unit_bundled_service.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "ledger/transaction_processor.hpp"
#include "miner/basic_miner.hpp"
#include "network/muddle/muddle.hpp"
#include "network/p2pservice/manifest.hpp"
#include "network/p2pservice/p2p_service.hpp"
#include "network/p2pservice/p2ptrust_bayrank.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <iterator>
#include <memory>
#include <random>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace fetch {

namespace ledger {
namespace consensus {

enum class ConsensusMinerType
{
  NO_MINER    = 0,
  DUMMY_MINER = 1,
  BAD_MINER   = 2
};

}  // namespace consensus
}  // namespace ledger

/**
 * Top level container for all components that are required to run a ledger instance
 */
class Constellation
{
public:
  using Peer2PeerService = p2p::P2PService;
  using CertificatePtr   = Peer2PeerService::CertificatePtr;
  using UriList          = std::vector<network::Uri>;
  using Manifest         = network::Manifest;

  static constexpr char const *LOGGING_NAME = "constellation";

  explicit Constellation(CertificatePtr &&certificate, Manifest &&manifest, uint32_t num_executors,
                         uint32_t log2_num_lanes, uint32_t num_slices,
                         std::string interface_address, std::string const &prefix,
                         std::string my_network_address, std::size_t processor_threads,
                         std::size_t                         verification_threads,
                         std::chrono::steady_clock::duration block_interval, std::size_t max_peers,
                         std::size_t transient_peers, uint32_t peer_update_cycle_ms,
                         uint32_t p2p_process_cycle_ms, uint32_t p2p_manifest_update_cycle_ms,
                         uint32_t p2p_block_catchup_cycle);

  void Run(UriList const &initial_peers, ledger::consensus::ConsensusMinerType const &mining);
  void SignalStop();

private:
  void CreateInfoFile(std::string const &filename);

  using Muddle                 = muddle::Muddle;
  using NetworkManager         = network::NetworkManager;
  using BlockPackingAlgorithm  = miner::BasicMiner;
  using Miner                  = ledger::MainChainMiner;
  using BlockCoordinator       = ledger::BlockCoordinator;
  using MainChain              = ledger::MainChain;
  using MainChainRpcService    = ledger::MainChainRpcService;
  using MainChainRpcServicePtr = std::shared_ptr<MainChainRpcService>;
  using LaneServices           = ledger::StorageUnitBundledService;
  using StorageUnitClient      = ledger::StorageUnitClient;
  using LaneIndex              = StorageUnitClient::LaneIndex;
  using StorageUnitClientPtr   = std::shared_ptr<StorageUnitClient>;
  using Flag                   = std::atomic<bool>;
  using ExecutionManager       = ledger::ExecutionManager;
  using ExecutionManagerPtr    = std::shared_ptr<ExecutionManager>;
  using LaneRemoteControl      = ledger::LaneRemoteControl;
  using HttpServer             = http::HTTPServer;
  using HttpModule             = http::HTTPModule;
  using HttpModulePtr          = std::shared_ptr<HttpModule>;
  using HttpModules            = std::vector<HttpModulePtr>;
  using TransactionProcessor   = ledger::TransactionProcessor;
  using TrustSystem            = p2p::P2PTrustBayRank<Muddle::Address>;
  using ConsensusMinerInterface =
      std::shared_ptr<fetch::ledger::consensus::ConsensusMinerInterface>;

  /// @name Configuration
  /// @{
  Flag        active_;             ///< Flag to control running of main thread
  Manifest    manifest_;           ///< The service manifest
  std::string interface_address_;  ///< The publicly facing interface IP address
  uint32_t    num_lanes_;          ///< The configured number of lanes
  uint32_t    num_slices_;         ///< The configured number of slices per block
  uint16_t    p2p_port_;           ///< The port that the P2P interface is running from
  uint16_t    http_port_;          ///< The port of the HTTP server
  uint16_t    lane_port_start_;    ///< The starting port of all the lane services
  /// @}

  /// @name Network Orchestration
  /// @{
  NetworkManager   network_manager_;       ///< Top level network coordinator
  NetworkManager   http_network_manager_;  ///< A separate net. coordinator for the http service(s)
  Muddle           muddle_;                ///< The muddle networking service
  TrustSystem      trust_;                 ///< The trust subsystem
  Peer2PeerService p2p_;                   ///< The main p2p networking stack
  /// @}

  /// @name Transaction and State Database shards
  /// @{
  LaneServices         lane_services_;  ///< The lane services
  StorageUnitClientPtr storage_;        ///< The storage client to the lane services
  LaneRemoteControl    lane_control_;   ///< The lane control client for the lane services
  /// @}

  /// @name Block Processing
  /// @{
  ExecutionManagerPtr execution_manager_;  ///< The transaction execution manager
  /// @}

  /// @name Blockchain and Mining
  /// @[
  MainChain               chain_;              ///< The main block chain component
  BlockPackingAlgorithm   block_packer_;       ///< The block packing / mining algorithm
  BlockCoordinator        block_coordinator_;  ///< The block execution coordinator
  ConsensusMinerInterface consensus_miner_;
  Miner                   miner_;  ///< The miner and block generation component
  /// @}

  /// @name Top Level Services
  /// @{
  MainChainRpcServicePtr main_chain_service_;  ///< Service for block transmission over the network
  TransactionProcessor   tx_processor_;        ///< The transaction entrypoint
  /// @}

  /// @name HTTP Server
  /// @{
  HttpServer  http_;          ///< The HTTP server
  HttpModules http_modules_;  ///< The set of modules currently configured
  /// @}
};

/**
 * Signal that constellation should attempt to shutdown gracefully
 */
inline void Constellation::SignalStop()
{
  active_ = false;
}

}  // namespace fetch
