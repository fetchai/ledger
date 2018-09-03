#pragma once
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

#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/main_chain_miner.hpp"
#include "ledger/protocols/main_chain_rpc_service.hpp"
#include "miner/annealer_miner.hpp"
#include "network/p2pservice/p2p_service2.hpp"
#include "network/muddle/muddle.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <iterator>
#include <memory>
#include <random>
#include <thread>
#include <tuple>
#include <unordered_set>

namespace fetch {

class Constellation
{
public:
  using NetworkManager = network::NetworkManager;
  using Peer2PeerService  = p2p::P2PService2;
  using MuddleService  = muddle::Muddle;
  using CertificatePtr    = Peer2PeerService::CertificatePtr;
  using PeerList    = std::vector<network::Peer>;
  using BlockPackingAlgorithm = miner::AnnealerMiner;
  using Miner = chain::MainChainMiner;
  using BlockCoordinator = chain::BlockCoordinator;
  using MainChain = chain::MainChain;
  using ExecutionManager = ledger::ExecutionManager;
  using ExecutionManagerPtr = std::shared_ptr<ExecutionManager>;

  using MainChainRpcService = ledger::MainChainRpcService;
  using MainChainRpcServicePtr = std::shared_ptr<MainChainRpcService>;

#if 0

  using executor_type          = std::shared_ptr<ledger::Executor>;
  using executor_list_type     = std::vector<executor_type>;
  using storage_client_type    = std::shared_ptr<ledger::StorageUnitClient>;
  using connection_type        = network::TCPClient;
  using execution_manager_type = std::shared_ptr<ledger::ExecutionManager>;
  using storage_service_type   = ledger::StorageUnitBundledService;
  using flag_type              = std::atomic<bool>;

  using block_coordinator_type = std::unique_ptr<chain::BlockCoordinator>;
  using chain_miner_type       = std::unique_ptr<chain::MainChainMiner>;

  using clock_type        = std::chrono::high_resolution_clock;
  using timepoint_type    = clock_type::time_point;
  using string_list_type  = std::vector<std::string>;
  using muddle_service_type  = std::shared_ptr<muddle::Muddle>;
  using http_server_type  = std::unique_ptr<http::HTTPServer>;
  using http_module_type  = std::shared_ptr<http::HTTPModule>;
  using http_modules_type = std::vector<http_module_type>;
  using miner_type        = std::unique_ptr<miner::MinerInterface>;
  using tx_processor_type = std::unique_ptr<ledger::TransactionProcessor>;

  using main_chain_service_type = std::unique_ptr<chain::MainChainService>;
  using main_chain_remote_type  = std::unique_ptr<chain::MainChainRemoteControl>;

  using service_type        = service::ServiceClient;
  using client_type         = fetch::network::TCPClient;
  using shared_service_type = std::shared_ptr<service_type>;
#endif

  static constexpr uint16_t    MAIN_CHAIN_PORT_OFFSET = 2;
  static constexpr uint16_t    P2P_PORT_OFFSET        = 1;
  static constexpr uint16_t    HTTP_PORT_OFFSET       = 0;
  static constexpr uint16_t    STORAGE_PORT_OFFSET    = 10;
  static constexpr uint32_t    DEFAULT_MINING_TARGET  = 10;
  static constexpr uint32_t    DEFAULT_IDLE_SPEED     = 2000;
  static constexpr uint16_t    DEFAULT_PORT_START     = 5000;
  static constexpr std::size_t DEFAULT_NUM_LANES      = 8;
  static constexpr std::size_t DEFAULT_NUM_SLICES     = 4;
  static constexpr std::size_t DEFAULT_NUM_EXECUTORS  = DEFAULT_NUM_LANES;
  //  static const std::string DEFAULT_DB_PREFIX =;
  static constexpr char const *LOGGING_NAME = "constellation";


  explicit Constellation(CertificatePtr &&certificate, uint16_t port_start = DEFAULT_PORT_START,
                         std::size_t        num_executors     = DEFAULT_NUM_EXECUTORS,
                         std::size_t        num_lanes         = DEFAULT_NUM_LANES,
                         std::size_t        num_slices        = DEFAULT_NUM_SLICES,
                         std::string const &interface_address = "127.0.0.1",
                         std::string const &prefix            = "node_storage");

  void Run(PeerList const &initial_peers = PeerList{});
  void SignalStop()
  {
    active_ = false;
  }

#if 0
  executor_type CreateExecutor()
  {
    FETCH_LOG_DEBUG(LOGGING_NAME,"Creating local executor...");
    return executor_type{new ledger::Executor(storage_)};
  }
#endif

private:

  using Flag = std::atomic<bool>;

  /// @name Configuration
  /// @{
  Flag        active_;                     ///< Flag to control running of main thread
  std::string interface_address_;          ///< The publicly facing interface IP address
  uint32_t    num_lanes_;                  ///< The configured number of lanes
  uint32_t    num_slices_;                 ///< The configured number of slices per block
  uint16_t    p2p_port_;                   ///< The port that the P2P interface is running from
  uint16_t    http_port_;                  ///< The port of the HTTP server
  uint16_t    lane_port_start_;            ///< The starting port of all the lane services
  uint16_t    main_chain_port_;            ///< The main chain port
  /// @}

  /// @name Network Orchestration
  /// @{
  NetworkManager    network_manager_;       ///< Top level network coordinator
  MuddleService  muddle_;                   ///
  Peer2PeerService  p2p_;                   ///< The main p2p networking
  /// @}

  /// @name Block processing
  ExecutionManagerPtr execution_manager_;

  /// @name Blockchain and Mining
  /// @[
  MainChain             chain_;
  BlockPackingAlgorithm block_packer_;      ///< The block packing / mining algorithm
  BlockCoordinator      block_coordinator_; ///< The block co-ordinator which controls the execution manager
  Miner                 miner_;
  /// @}

  /// @name Top Level Services
  /// @{
  MainChainRpcServicePtr   main_chain_service_;
  /// @}

#if 0
  /// @name Lane Storage Components
  /// @{
  storage_service_type storage_service_;  ///< The combination of all the lane services
  storage_client_type  storage_;          ///< The storage client
  /// @}

  /// @name Block Execution
  /// @{
  executor_list_type     executors_;          ///< The list of transaction executors
  execution_manager_type execution_manager_;  ///< The execution manager
  /// @}

  /// @name Blockchain Components
  /// @{
  main_chain_service_type main_chain_service_;  ///< The main chain
  main_chain_remote_type  main_chain_remote_;   ///< The controller unit of the main chain

  tx_processor_type      tx_processor_;        ///< The transaction processor
  block_coordinator_type block_coordinator_;   ///< The block coordinator
  miner_type             transaction_packer_;  ///< The colourful puzzle solver
  chain_miner_type       main_chain_miner_;    ///< The main chain miner
  /// @}

  /// @name P2P Networking Components
  /// @{
  p2p_service_type p2p_;  ///< The P2P networking component
  muddle_service_type muddle_;  ///< The muddle networking component
  /// @}

  /// @name Muddle Networking Components
  /// @{
  muddle_service_type muddle_;  ///< The muddle networking component
  /// @}

  /// @name API Components
  /// @{
  http_server_type  http_;          ///< The HTTP interfaces server
  http_modules_type http_modules_;  ///< The list of registered HTTP modules
  /// @}
#endif
};

}  // namespace fetch
